#include "signature.h"
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/bn.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "aesm.pb.h"
#include "logging.h"

#define SIGSTRUCT_RESERVED_SIZE1 84
#define MASK_INIT_VALUE 0xffffffffffffffff

using namespace std;

SigstructGenerator::SigstructGenerator(secs_t *secs) {
    scs = secs;
    SHA256_Init(&c);
    this->readPublicKeyFile("publickey");
    this->readPrivateKeyFile("privatekey");
    assert(!this->privateKey.empty() && !this->publicKey.empty());
}
void SigstructGenerator::doEcreate(uint64_t size, uint32_t nframes) {
    Ecreate ecreate = {};
    char cmd[] = "ECREATE";
    memcpy(ecreate.cmd, cmd, strlen(cmd));
    ecreate.fst = nframes;
    ecreate.size = size;
    memset(ecreate.suffix, 0, 44);
    SHA256_Update(&c, &ecreate, sizeof(struct Ecreate));
}

void SigstructGenerator::doEadd(uint64_t offset, sec_info_t &sec_info) {
    Eadd eadd = {};
    char cmd[] = "EADD";
    memcpy(eadd.cmd, cmd, strlen(cmd));
    eadd.offset = offset;
    eadd.flags = sec_info.flags;
    memset(eadd.suffix, 0, 40);
    SHA256_Update(&c, &eadd, sizeof(struct Eadd));
}

void SigstructGenerator::digestFinal() { SHA256_Final(m, &c); }

sigstruct *SigstructGenerator::getSigstruct() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char *signBuffer = (char *)malloc(4096);
    char *p = signBuffer;
    RSA *pub = generatePubRSA(publicKey);
    const BIGNUM *modulusBN = BN_new();
    RSA_get0_key(pub, &modulusBN, NULL, NULL); 
    
    BIGNUM *signatureBN = NULL;
    BIGNUM *tmp1 = BN_new();
    BIGNUM *tmp2 = BN_new();
    BIGNUM *q1BN = BN_new();
    BIGNUM *q2BN = BN_new();
    BIGNUM *q2TMP = BN_new();
    BIGNUM *rem2 = BN_new();
    BN_CTX *ctx = BN_CTX_new();
    unsigned short year = 1900 + ltm->tm_year;
    unsigned char month = (1 + ltm->tm_mon);
    unsigned char day = ltm->tm_mday;
    uint32_t ymd = year;
    ymd <<= 8;
    ymd += month;
    ymd <<= 8;
    ymd += day;
    //sstruct.header11 = 0x0000010000000000;
    //sstruct.header12 = 0x06000000E1000000;
    sstruct.header11 = 0x000000e100000006ULL;
    sstruct.header12 = 0x0000000000010000ULL;
    sstruct.vendor = 0x00000000;
    sstruct.date = ymd;
    //sstruct.header21 = 0x6000000001000000;
    //sstruct.header22 = 0x0101000060000000;
    sstruct.header21 = 0x0000006000000101ULL;
    sstruct.header22 = 0x0000000100000060ULL;
    
    sstruct.swdefined = 0x00000000;
    memset(sstruct.reserved1, 0, sizeof(uint8_t) * SIGSTRUCT_RESERVED_SIZE1);
    sstruct.miscselect = scs->miscselect;
    sstruct.micsmask = 0; // not sure, what is the mask?
    sstruct.reserved2 = 0;
    sstruct.isvfamilyid1 = 0; // not sure
    sstruct.isvfamilyid2 = 0;
    sstruct.attributes1 = scs->attributes;
    sstruct.attributes2 = scs->xfrm;
    sstruct.attributemask1 = MASK_INIT_VALUE;
    sstruct.attributemask2 = MASK_INIT_VALUE;
    memcpy(sstruct.enclavehash, m, sizeof(char) * SHA256_DIGEST_LENGTH);
    memset(sstruct.reserved3, 0, sizeof(uint8_t) * 16);
    sstruct.isvextprodid1 = 0; // no idea
    sstruct.isvextprodid2 = 0; //? dont know
    sstruct.isvprodid = scs->isvprodid;
    sstruct.isvsvn = scs->isvsvn;
    memset(sstruct.reserved4, 0, sizeof(uint8_t) * 12);
    
    memcpy(p, &sstruct.header11, sizeof(uint8_t) * 128);
    p += 128;
    memcpy(p, &sstruct.miscselect, sizeof(uint8_t) * 128);
    p += 128;

    console->info("p == signBuffer + {}", p - signBuffer);
    assert(p == signBuffer + 256);
    string plainTxt(signBuffer, p);
    string signature = signMsg(plainTxt);
    assert(signature.length() == 384);
    if (!(signatureBN = BN_bin2bn((unsigned char *)signature.data(), signature.length(), NULL))) {
        console->critical("BN_lebin2bn failed: {}", ERR_error_string(ERR_get_error(), NULL));
    }

    BN_mul(tmp1, signatureBN, signatureBN, ctx);
    BN_div(q1BN, tmp2, tmp1, modulusBN, ctx);
    BN_mul(q2TMP, tmp2, signatureBN, ctx);
    BN_div(q2BN, rem2, q2TMP, modulusBN, ctx);
    //q1 = BN_bn2hex(q1BN);
    auto *q1 = new unsigned char[BN_num_bytes(q1BN)];
    BN_bn2lebinpad(q1BN, q1, BN_num_bytes(q1BN));
    
    //q2 = BN_bn2hex(q2BN);
    auto *q2 = new unsigned char[BN_num_bytes(q2BN)];
    BN_bn2lebinpad(q2BN, q2, BN_num_bytes(q2BN));

    //modulus = BN_bn2hex(modulusBN);
    auto *modulus = new unsigned char[BN_num_bytes(modulusBN)];
    assert(BN_num_bytes(modulusBN) == 384);
    BN_bn2lebinpad(modulusBN, modulus, BN_num_bytes(modulusBN));

    auto *signature_le = new unsigned char[BN_num_bytes(signatureBN)];
    assert(BN_num_bytes(signatureBN) == 384);
    BN_bn2lebinpad(signatureBN, signature_le, BN_num_bytes(signatureBN));

    memcpy(sstruct.modulus, modulus, sizeof(uint8_t) * 384);
    sstruct.exponent = 3;
    memcpy(sstruct.signature, signature_le, sizeof(uint8_t) * 384);
    memcpy(sstruct.q1, q1, sizeof(uint8_t) * 384);
    memcpy(sstruct.q2, q2, sizeof(uint8_t) * 384);
    free(signBuffer);
    return &sstruct;
}

RSA *SigstructGenerator::generatePriRSA(string key) {
    RSA *rsa = NULL;
    const char *c_string = key.c_str();
    BIO *keybio = BIO_new_mem_buf((void *)c_string, -1);
    if (keybio == NULL) {
        return 0;
    }
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    return rsa;
}

RSA *SigstructGenerator::generatePubRSA(string key) {
    RSA *rsa = NULL;
    BIO *keybio;
    const char *c_string = key.c_str();
    keybio = BIO_new_mem_buf((void *)c_string, -1);
    if (keybio == NULL) {
        return 0;
    }
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, NULL, NULL, NULL);
    if (!rsa)
        console->critical("generatePubRSA failed {}", ERR_error_string(ERR_get_error(), NULL));
    return rsa;
}

int SigstructGenerator::RSASign(RSA *rsa, const unsigned char *Msg,
                                size_t MsgLen, unsigned char **EncMsg,
                                size_t *EncMsgLen) {
    EVP_MD_CTX *m_RSASignCtx = EVP_MD_CTX_create();
    EVP_PKEY *priKey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(priKey, rsa);
    if (EVP_DigestSignInit(m_RSASignCtx, NULL, EVP_sha256(), NULL, priKey) <=
        0) {
        return false;
    }
    if (EVP_DigestSignUpdate(m_RSASignCtx, Msg, MsgLen) <= 0) {
        return false;
    }
    if (EVP_DigestSignFinal(m_RSASignCtx, NULL, EncMsgLen) <= 0) {
        return false;
    }
    *EncMsg = (unsigned char *)malloc(*EncMsgLen);
    if (EVP_DigestSignFinal(m_RSASignCtx, *EncMsg, EncMsgLen) <= 0) {
        return false;
    }
    EVP_MD_CTX_free(m_RSASignCtx);
    return true;
}

string SigstructGenerator::signMsg(string plainText) {
    RSA *privateRSA = generatePriRSA(privateKey);
    unsigned char *encMsg;
    size_t encMessageLength;
    RSASign(privateRSA, (unsigned char *)plainText.c_str(), plainText.length(),
            &encMsg, &encMessageLength);

    string ret(encMsg, encMsg + encMessageLength);
    free(encMsg);
    return ret;
}

static string readFileToString(const string &filename) {
    ifstream ins(filename);
    if (!ins.is_open()) {
        console->critical("Cannot open key file {}", filename);
        return "";
    }
    stringstream buffer;
    buffer << ins.rdbuf();
    return buffer.str();
}

void SigstructGenerator::readPrivateKeyFile(const string &filename) {
    this->privateKey = readFileToString(filename);
}

void SigstructGenerator::readPublicKeyFile(const string &filename) {
    this->publicKey = readFileToString(filename);
}

//=======================================================
// the following code is responsible for getting the
// launch token from intel's psw service
//

TokenGetter::TokenGetter(const string &filename) {
    struct sockaddr_un serveraddr = {};

    this->sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->sockfd < 0) {
        console->error("Cannot open socket to aems");
        exit(-1);
    }

    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, filename.c_str());

    if (connect(this->sockfd, (struct sockaddr *)&serveraddr,
                SUN_LEN(&serveraddr)) < 0) {
        console->error("Cannot connect to aems {}", strerror(errno));
        exit(-1);
    }

    console->trace("Successfully connected to aems at {}", filename);
}

/*
static string sha256_string(const string &str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.data(), str.length());
    SHA256_Final(hash, &sha256);
    
    return string(hash, hash + SHA256_DIGEST_LENGTH);
}
*/

const einittoken_t *TokenGetter::getToken(const sigstruct *sig) {
    string tmp;

    aesm::message::Request request;
    request.mutable_getlictokenreq()->set_mr_enclave(sig->enclavehash, 32);
    request.mutable_getlictokenreq()->set_mr_signer(sig->modulus, 384);
    request.mutable_getlictokenreq()->set_se_attributes(&sig->attributes1, 16);
    request.mutable_getlictokenreq()->set_timeout(1000);
    console->debug("Dump protobuf send message: {}", request.DebugString());
    
    string sendBuf = request.SerializeAsString(); 
    uint32_t sendLen = sendBuf.length();
    console->trace("aems: sendLen = {}", sendLen);
    if (write(this->sockfd, &sendLen, 4) != 4) {
        console->error("Cannot write sendLen to aems");
        exit(-1);
    }
    size_t beginInd = 0;
    while (beginInd < sendBuf.length()) {
        ssize_t nbytes =
            write(this->sockfd, sendBuf.data() + beginInd, sendBuf.length() - beginInd);
        console->info("wrote {} bytes to aesmd", nbytes);
        if (nbytes <= 0) {
            console->error("Cannot write to aems socket {}", strerror(errno));
            exit(-1);
        }
        beginInd += nbytes;
    }

    uint32_t recvLen = 0;
    if (read(this->sockfd, &recvLen, 4) != 4) {
        console->error("Cannot read recvLen from aems");
        exit(-1);
    }
    console->trace("aems: recvLen = {}", recvLen);

    char *recvBuf = new char[recvLen];
    beginInd = 0;
    while (beginInd < recvLen) {
        ssize_t nbytes =
            read(this->sockfd, recvBuf + beginInd, recvLen - beginInd);
        if (nbytes < 0) {
            console->error("Cannot read from aems socket {}", strerror(errno));
            exit(-1);
        }
        beginInd += nbytes;
    }

    aesm::message::Response response;
    response.ParseFromArray(recvBuf, recvLen);
    console->debug("Dump protobuf recv message: {}", response.DebugString());

    if (!response.getlictokenres().has_errorcode())
        console->critical("aesmd didn't give an errorcode!");
    auto err = response.getlictokenres().errorcode();
    if (err) {
        console->error("Cannot get launch token: {}", err);
        exit(-1);
    }

    tmp = response.getlictokenres().token();
    console->info("token size = {}, should be = {}", tmp.length(), sizeof(einittoken_t));
    memcpy(&this->token, tmp.data(), sizeof(einittoken_t));

    this->dumpToken();
    return &this->token;
}

void TokenGetter::dumpToken() {
    console->trace("token dump:");
    console->trace("valid: {}", this->token.valid);
}

TokenGetter::~TokenGetter() {
    if (this->sockfd >= 0)
        close(this->sockfd);
}
