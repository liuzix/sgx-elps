#!/usr/bin/perl

use strict;
use warnings;
use Test::More;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;
use Data::Dumper qw/Dumper/;

my $ext_path;

if (!supports_extstore()) {
    plan skip_all => 'extstore not enabled';
    exit 0;
}

$ext_path = "/tmp/extstore.$$";

my $server = new_memcached("-m 64 -U 0 -o ext_page_size=8,ext_wbuf_size=2,ext_threads=1,ext_io_depth=2,ext_item_size=512,ext_item_age=2,ext_recache_rate=10000,ext_max_frag=0.9,ext_path=$ext_path:64m,no_lru_crawler,slab_automove=0");
ok($server, "started the server");

# Based almost 100% off testClient.py which is:
# Copyright (c) 2007  Dustin Sallings <dustin@spy.net>

# Command constants
use constant CMD_GET        => 0x00;
use constant CMD_SET        => 0x01;
use constant CMD_ADD        => 0x02;
use constant CMD_REPLACE    => 0x03;
use constant CMD_DELETE     => 0x04;
use constant CMD_INCR       => 0x05;
use constant CMD_DECR       => 0x06;
use constant CMD_QUIT       => 0x07;
use constant CMD_FLUSH      => 0x08;
use constant CMD_GETQ       => 0x09;
use constant CMD_NOOP       => 0x0A;
use constant CMD_VERSION    => 0x0B;
use constant CMD_GETK       => 0x0C;
use constant CMD_GETKQ      => 0x0D;
use constant CMD_APPEND     => 0x0E;
use constant CMD_PREPEND    => 0x0F;
use constant CMD_STAT       => 0x10;
use constant CMD_SETQ       => 0x11;
use constant CMD_ADDQ       => 0x12;
use constant CMD_REPLACEQ   => 0x13;
use constant CMD_DELETEQ    => 0x14;
use constant CMD_INCREMENTQ => 0x15;
use constant CMD_DECREMENTQ => 0x16;
use constant CMD_QUITQ      => 0x17;
use constant CMD_FLUSHQ     => 0x18;
use constant CMD_APPENDQ    => 0x19;
use constant CMD_PREPENDQ   => 0x1A;
use constant CMD_TOUCH      => 0x1C;
use constant CMD_GAT        => 0x1D;
use constant CMD_GATQ       => 0x1E;
use constant CMD_GATK       => 0x23;
use constant CMD_GATKQ      => 0x24;

# REQ and RES formats are divided even though they currently share
# the same format, since they _could_ differ in the future.
use constant REQ_PKT_FMT      => "CCnCCnNNNN";
use constant RES_PKT_FMT      => "CCnCCnNNNN";
use constant INCRDECR_PKT_FMT => "NNNNN";
use constant MIN_RECV_BYTES   => length(pack(RES_PKT_FMT));
use constant REQ_MAGIC        => 0x80;
use constant RES_MAGIC        => 0x81;

my $mc = MC::Client->new;

my $check = sub {
    my ($key, $orig_flags, $orig_val) = @_;
    my ($flags, $val, $cas) = $mc->get($key);
    is($flags, $orig_flags, "Flags is set properly");
    ok($val eq $orig_val || $val == $orig_val, $val . " = " . $orig_val);
};

my $set = sub {
    my ($key, $exp, $orig_flags, $orig_value) = @_;
    $mc->set($key, $orig_value, $orig_flags, $exp);
    $check->($key, $orig_flags, $orig_value);
};

my $empty = sub {
    my $key = shift;
    my $rv =()= eval { $mc->get($key) };
    is($rv, 0, "Didn't get a result from get");
    ok($@->not_found, "We got a not found error when we expected one");
};

my $delete = sub {
    my ($key, $when) = @_;
    $mc->delete($key, $when);
    $empty->($key);
};

my $value;
my $bigvalue;
{
    my @chars = ("C".."Z");
    for (1 .. 20000) {
        $value .= $chars[rand @chars];
    }
    for (1 .. 800000) {
        $bigvalue .= $chars[rand @chars];
    }
}

# diag "small object";
$set->('x', 10, 19, "somevalue");

# check extstore counters
{
    my %stats = $mc->stats('');
    is($stats{extstore_objects_written}, 0);
}

# diag "Delete";
#$delete->('x');

# diag "Flush";
#$empty->('y');

# fill some larger objects
{
    my $keycount = 1000;
    for (1 .. $keycount) {
        $set->("nfoo$_", 0, 19, $value);
    }
    # wait for a flush
    sleep 4;
    # value returns for one flushed object.
    $check->('nfoo1', 19, $value);

    # check extstore counters
    my %stats = $mc->stats('');
    cmp_ok($stats{extstore_page_allocs}, '>', 0, 'at least one page allocated');
    cmp_ok($stats{extstore_objects_written}, '>', $keycount / 2, 'some objects written');
    cmp_ok($stats{extstore_bytes_written}, '>', length($value) * 2, 'some bytes written');
    cmp_ok($stats{get_extstore}, '>', 0, 'one object was fetched');
    cmp_ok($stats{extstore_objects_read}, '>', 0, 'one object read');
    cmp_ok($stats{extstore_bytes_read}, '>', length($value), 'some bytes read');
    # Test multiget
    my $rv = $mc->get_multi(qw(nfoo2 nfoo3 noexist));
    is($rv->{nfoo2}->[1], $value, 'multiget nfoo2');
    is($rv->{nfoo3}->[1], $value, 'multiget nfoo2');

    # Remove half of the keys for the next test.
    for (1 .. $keycount) {
        next unless $_ % 2 == 0;
        $delete->("nfoo$_");
    }

    my %stats2 = $mc->stats('');
    cmp_ok($stats{extstore_bytes_used}, '>', $stats2{extstore_bytes_used},
        'bytes used dropped after deletions');
    cmp_ok($stats{extstore_objects_used}, '>', $stats2{extstore_objects_used},
        'objects used dropped after deletions');
    is($stats2{badcrc_from_extstore}, 0, 'CRC checks successful');

    # delete the rest
    for (1 .. $keycount) {
        next unless $_ % 2 == 1;
        $delete->("nfoo$_");
    }
}

# check evictions and misses
{
    my $keycount = 1000;
    for (1 .. $keycount) {
        $set->("mfoo$_", 0, 19, $value);
    }
    sleep 4;
    for ($keycount .. ($keycount*3)) {
        $set->("mfoo$_", 0, 19, $value);
    }
    sleep 4;
    # FIXME: Need to sample through a few values, or fix eviction to be
    # more accurate. On 32bit systems some pages unused to this point get
    # filled after the first few items, then the eviction algo pulls those
    # pages since they have the lowest version number, leaving older objects
    # in memory and evicting newer ones.
    for (1 .. ($keycount*3)) {
        next unless $_ % 100 == 0;
        eval { $mc->get("mfoo$_"); };
    }

    my %s = $mc->stats('');
    cmp_ok($s{extstore_objects_evicted}, '>', 0);
    cmp_ok($s{miss_from_extstore}, '>', 0);
}

# store and re-fetch a chunked value
{
    my %stats = $mc->stats('');
    $set->("bigvalue", 0, 0, $bigvalue);
    sleep 4;
    $check->("bigvalue", 0, $bigvalue);
    my %stats2 = $mc->stats('');

    cmp_ok($stats2{extstore_objects_written}, '>',
        $stats{extstore_objects_written}, "a large value flushed");
}

# ensure ASCII can still fetch the chunked value.
{
    my $ns = $server->new_sock;

    my %s1 = $mc->stats('');
    mem_get_is($ns, "bigvalue", $bigvalue);
    print $ns "extstore recache_rate 1\r\n";
    is(scalar <$ns>, "OK\r\n", "recache rate upped");
    for (1..3) {
        mem_get_is($ns, "bigvalue", $bigvalue);
        $check->('bigvalue', 0, $bigvalue);
    }
    my %s2 = $mc->stats('');
    cmp_ok($s2{recache_from_extstore}, '>', $s1{recache_from_extstore},
        'a new recache happened');

}

done_testing();

END {
    unlink $ext_path if $ext_path;
}
# ######################################################################
# Test ends around here.
# ######################################################################

package MC::Client;

use strict;
use warnings;
use fields qw(socket);
use IO::Socket::INET;

sub new {
    my $self = shift;
    my ($s) = @_;
    $s = $server unless defined $s;
    my $sock = $s->sock;
    $self = fields::new($self);
    $self->{socket} = $sock;
    return $self;
}

sub build_command {
    my $self = shift;
    die "Not enough args to send_command" unless @_ >= 4;
    my ($cmd, $key, $val, $opaque, $extra_header, $cas) = @_;

    $extra_header = '' unless defined $extra_header;
    my $keylen    = length($key);
    my $vallen    = length($val);
    my $extralen  = length($extra_header);
    my $datatype  = 0;  # field for future use
    my $reserved  = 0;  # field for future use
    my $totallen  = $keylen + $vallen + $extralen;
    my $ident_hi  = 0;
    my $ident_lo  = 0;

    if ($cas) {
        $ident_hi = int($cas / 2 ** 32);
        $ident_lo = int($cas % 2 ** 32);
    }

    my $msg = pack(::REQ_PKT_FMT, ::REQ_MAGIC, $cmd, $keylen, $extralen,
                   $datatype, $reserved, $totallen, $opaque, $ident_hi,
                   $ident_lo);
    my $full_msg = $msg . $extra_header . $key . $val;
    return $full_msg;
}

sub send_command {
    my $self = shift;
    die "Not enough args to send_command" unless @_ >= 4;
    my ($cmd, $key, $val, $opaque, $extra_header, $cas) = @_;

    my $full_msg = $self->build_command($cmd, $key, $val, $opaque, $extra_header, $cas);

    my $sent = $self->{socket}->send($full_msg);
    die("Send failed:  $!") unless $sent;
    if($sent != length($full_msg)) {
        die("only sent $sent of " . length($full_msg) . " bytes");
    }
}

sub flush_socket {
    my $self = shift;
    $self->{socket}->flush;
}

# Send a silent command and ensure it doesn't respond.
sub send_silent {
    my $self = shift;
    die "Not enough args to send_silent" unless @_ >= 4;
    my ($cmd, $key, $val, $opaque, $extra_header, $cas) = @_;

    $self->send_command($cmd, $key, $val, $opaque, $extra_header, $cas);
    $self->send_command(::CMD_NOOP, '', '', $opaque + 1);

    my ($ropaque, $data) = $self->_handle_single_response;
    Test::More::is($ropaque, $opaque + 1);
}

sub silent_mutation {
    my $self = shift;
    my ($cmd, $key, $value) = @_;

    $empty->($key);
    my $extra = pack "NN", 82, 0;
    $mc->send_silent($cmd, $key, $value, 7278552, $extra, 0);
    $check->($key, 82, $value);
}

sub _handle_single_response {
    my $self = shift;
    my $myopaque = shift;

    my $hdr = "";
    while(::MIN_RECV_BYTES - length($hdr) > 0) {
        $self->{socket}->recv(my $response, ::MIN_RECV_BYTES - length($hdr));
        $hdr .= $response;
    }
    Test::More::is(length($hdr), ::MIN_RECV_BYTES, "Expected read length");

    my ($magic, $cmd, $keylen, $extralen, $datatype, $status, $remaining,
        $opaque, $ident_hi, $ident_lo) = unpack(::RES_PKT_FMT, $hdr);
    Test::More::is($magic, ::RES_MAGIC, "Got proper response magic");

    my $cas = ($ident_hi * 2 ** 32) + $ident_lo;

    return ($opaque, '', $cas, 0) if($remaining == 0);

    # fetch the value
    my $rv="";
    while($remaining - length($rv) > 0) {
        $self->{socket}->recv(my $buf, $remaining - length($rv));
        $rv .= $buf;
    }
    if(length($rv) != $remaining) {
        my $found = length($rv);
        die("Expected $remaining bytes, got $found");
    }
    if (defined $myopaque) {
        Test::More::is($opaque, $myopaque, "Expected opaque");
    } else {
        Test::More::pass("Implicit pass since myopaque is undefined");
    }

    if ($status) {
        die MC::Error->new($status, $rv);
    }

    return ($opaque, $rv, $cas, $keylen);
}

sub _do_command {
    my $self = shift;
    die unless @_ >= 3;
    my ($cmd, $key, $val, $extra_header, $cas) = @_;

    $extra_header = '' unless defined $extra_header;
    my $opaque = int(rand(2**32));
    $self->send_command($cmd, $key, $val, $opaque, $extra_header, $cas);
    my (undef, $rv, $rcas) = $self->_handle_single_response($opaque);
    return ($rv, $rcas);
}

sub _incrdecr_header {
    my $self = shift;
    my ($amt, $init, $exp) = @_;

    my $amt_hi = int($amt / 2 ** 32);
    my $amt_lo = int($amt % 2 ** 32);

    my $init_hi = int($init / 2 ** 32);
    my $init_lo = int($init % 2 ** 32);

    my $extra_header = pack(::INCRDECR_PKT_FMT, $amt_hi, $amt_lo, $init_hi,
                            $init_lo, $exp);

    return $extra_header;
}

sub _incrdecr_cas {
    my $self = shift;
    my ($cmd, $key, $amt, $init, $exp) = @_;

    my ($data, $rcas) = $self->_do_command($cmd, $key, '',
                                           $self->_incrdecr_header($amt, $init, $exp));

    my $header = substr $data, 0, 8, '';
    my ($resp_hi, $resp_lo) = unpack "NN", $header;
    my $resp = ($resp_hi * 2 ** 32) + $resp_lo;

    return $resp, $rcas;
}

sub _incrdecr {
    my $self = shift;
    my ($v, $c) = $self->_incrdecr_cas(@_);
    return $v
}

sub silent_incrdecr {
    my $self = shift;
    my ($cmd, $key, $amt, $init, $exp) = @_;
    my $opaque = 8275753;

    $mc->send_silent($cmd, $key, '', $opaque,
                     $mc->_incrdecr_header($amt, $init, $exp));
}

sub stats {
    my $self = shift;
    my $key  = shift;
    my $cas = 0;
    my $opaque = int(rand(2**32));
    $self->send_command(::CMD_STAT, $key, '', $opaque, '', $cas);

    my %rv = ();
    my $found_key = '';
    my $found_val = '';
    do {
        my ($op, $data, $cas, $keylen) = $self->_handle_single_response($opaque);
        if($keylen > 0) {
            $found_key = substr($data, 0, $keylen);
            $found_val = substr($data, $keylen);
            $rv{$found_key} = $found_val;
        } else {
            $found_key = '';
        }
    } while($found_key ne '');
    return %rv;
}

sub get {
    my $self = shift;
    my $key  = shift;
    my ($rv, $cas) = $self->_do_command(::CMD_GET, $key, '', '');

    my $header = substr $rv, 0, 4, '';
    my $flags  = unpack("N", $header);

    return ($flags, $rv, $cas);
}

sub get_multi {
    my $self = shift;
    my @keys = @_;

    for (my $i = 0; $i < @keys; $i++) {
        $self->send_command(::CMD_GETQ, $keys[$i], '', $i, '', 0);
    }

    my $terminal = @keys + 10;
    $self->send_command(::CMD_NOOP, '', '', $terminal);

    my %return;
    while (1) {
        my ($opaque, $data) = $self->_handle_single_response;
        last if $opaque == $terminal;

        my $header = substr $data, 0, 4, '';
        my $flags  = unpack("N", $header);

        $return{$keys[$opaque]} = [$flags, $data];
    }

    return %return if wantarray;
    return \%return;
}

sub touch {
    my $self = shift;
    my ($key, $expire) = @_;
    my $extra_header = pack "N", $expire;
    my $cas = 0;
    return $self->_do_command(::CMD_TOUCH, $key, '', $extra_header, $cas);
}

sub gat {
    my $self   = shift;
    my $key    = shift;
    my $expire = shift;
    my $extra_header = pack "N", $expire;
    my ($rv, $cas) = $self->_do_command(::CMD_GAT, $key, '', $extra_header);

    my $header = substr $rv, 0, 4, '';
    my $flags  = unpack("N", $header);

    return ($flags, $rv, $cas);
}

sub version {
    my $self = shift;
    return $self->_do_command(::CMD_VERSION, '', '');
}

sub flush {
    my $self = shift;
    return $self->_do_command(::CMD_FLUSH, '', '');
}

sub add {
    my $self = shift;
    my ($key, $val, $flags, $expire) = @_;
    my $extra_header = pack "NN", $flags, $expire;
    my $cas = 0;
    return $self->_do_command(::CMD_ADD, $key, $val, $extra_header, $cas);
}

sub set {
    my $self = shift;
    my ($key, $val, $flags, $expire, $cas) = @_;
    my $extra_header = pack "NN", $flags, $expire;
    return $self->_do_command(::CMD_SET, $key, $val, $extra_header, $cas);
}

sub _append_prepend {
    my $self = shift;
    my ($cmd, $key, $val, $cas) = @_;
    return $self->_do_command($cmd, $key, $val, '', $cas);
}

sub replace {
    my $self = shift;
    my ($key, $val, $flags, $expire) = @_;
    my $extra_header = pack "NN", $flags, $expire;
    my $cas = 0;
    return $self->_do_command(::CMD_REPLACE, $key, $val, $extra_header, $cas);
}

sub delete {
    my $self = shift;
    my ($key) = @_;
    return $self->_do_command(::CMD_DELETE, $key, '');
}

sub incr {
    my $self = shift;
    my ($key, $amt, $init, $exp) = @_;
    $amt = 1 unless defined $amt;
    $init = 0 unless defined $init;
    $exp = 0 unless defined $exp;

    return $self->_incrdecr(::CMD_INCR, $key, $amt, $init, $exp);
}

sub incr_cas {
    my $self = shift;
    my ($key, $amt, $init, $exp) = @_;
    $amt = 1 unless defined $amt;
    $init = 0 unless defined $init;
    $exp = 0 unless defined $exp;

    return $self->_incrdecr_cas(::CMD_INCR, $key, $amt, $init, $exp);
}

sub decr {
    my $self = shift;
    my ($key, $amt, $init, $exp) = @_;
    $amt = 1 unless defined $amt;
    $init = 0 unless defined $init;
    $exp = 0 unless defined $exp;

    return $self->_incrdecr(::CMD_DECR, $key, $amt, $init, $exp);
}

sub noop {
    my $self = shift;
    return $self->_do_command(::CMD_NOOP, '', '');
}

package MC::Error;

use strict;
use warnings;

use constant ERR_UNKNOWN_CMD  => 0x81;
use constant ERR_NOT_FOUND    => 0x1;
use constant ERR_EXISTS       => 0x2;
use constant ERR_TOO_BIG      => 0x3;
use constant ERR_EINVAL       => 0x4;
use constant ERR_NOT_STORED   => 0x5;
use constant ERR_DELTA_BADVAL => 0x6;

use overload '""' => sub {
    my $self = shift;
    return "Memcache Error ($self->[0]): $self->[1]";
};

sub new {
    my $class = shift;
    my $error = [@_];
    my $self = bless $error, (ref $class || $class);

    return $self;
}

sub not_found {
    my $self = shift;
    return $self->[0] == ERR_NOT_FOUND;
}

sub exists {
    my $self = shift;
    return $self->[0] == ERR_EXISTS;
}

sub too_big {
    my $self = shift;
    return $self->[0] == ERR_TOO_BIG;
}

sub delta_badval {
    my $self = shift;
    return $self->[0] == ERR_DELTA_BADVAL;
}

sub einval {
    my $self = shift;
    return $self->[0] == ERR_EINVAL;
}

# vim: filetype=perl

