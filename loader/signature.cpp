#include "signature.h"
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
    privateKey =
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIIEpQIBAAKCAQEAxAswxiZfE/mHVI2nhdKJ1UAoTGRNXkExrs5prQJ7rATiKWM9\n"
        "L+s/df7nsfShZudLmpzlBQPUxi38ucVpvC5hn5D9whBuk2IVWF+yh3GEx1/9/iZU\n"
        "EdntrNRGIku4TGKrOAu9aYp+6o2HCkqw6v1ryie3xvsFbUQBh/ngwG8p53BlUeTO\n"
        "5lDCHCmlBK9FTTZwPs4856sa9VC+GZ7fEXW3WRJu94mKuFrtyWLNt60fu2izqd66\n"
        "IVNxm1rrDbOyEIind2hRaUJm3bv6cCoHLsr0aw9PK6VHqrRh9SaKjUH3U0nBEcAX\n"
        "m4OfavPQv4SuGdAiormBscWDNU1V+76A96hndQIDAQABAoIBAFF0TNmny81i2kXZ\n"
        "bYd+yOIf/B5xdmv5LolxHhtScswwYz/LCftFBWMjfGVGPWGiLJmbItUImHmXVfBi\n"
        "A+K6arl/5s+hQDBginnjmSwJkJ++VKwqhLe+ErDCqjlJuNOUE4v/5L1bXAxcrYUr\n"
        "L4MTtJuUERnN5p1VqkUzH50VExYjtNcrU8v4pvU+fz+rmYSuDb8Hge7TOOKN9ven\n"
        "iDCnU1Wb2pz79Q5fuoblbmeRu9ivgnTSBVbd05o0b+NeU4htmSgyom8u7UrIdUno\n"
        "s8jsd1ekrYbcCZSzOsw0eAxtGdKwjuljRyeOb5oa21YLhSEIKBLNvTGNaYqKr1PH\n"
        "Ut15LYECgYEA5q4jlxXs+dyiijNOZplGtZB2XxWNFUWLFW7gUyF0ZI2umJlxPWNi\n"
        "ftXpfe2JcvigVnfnYpyGHfi9hFu3ZHIvGyJetKx4IDrs30+AwcI03BksCmdl4CG+\n"
        "rqXtmcEauBZ+9Y+1u/8CgCPRKUScw5dXJn3W9snjbn6UBXPDBEGkrdECgYEA2Y/N\n"
        "rNlkYAtHqNVzXFcq4PjdntjbSzivoGDmrFoOtguvMmY7Lk3f2YT4YzQXfwJ+GeDK\n"
        "QFOaXeHnmJmFZ5jPpwPnzIRedMyvZ16UMjYTHLzM3Q4D3uNf8OAHlLY37DmhNPks\n"
        "VFOqjXnXmUaIN7VvVCaippcU83O6N+UpssfDlGUCgYEAswT5gcr2OYJMccwXT/Ar\n"
        "u8P82RW0g4mQwnVliZ4w4chCcxLBms6CJcSEi8HIJX1lkTVEUHcAbkoXaZxz2nPy\n"
        "srAdp0EhiIGySis81SGOPDEcyIYtvZ7yiD8lAWmm/q4WoSOB+f+RRTiGnewtbG0K\n"
        "qUiHhsZuxdVdsk2ATtFSp8ECgYEAirkC8D/9nLAUlTblQ+/gy2pkBbFIwwH2GlEv\n"
        "RJ532uRAZeaBvdix70S2DKtegAHa3i1TSQkF2O7+eXMKeTAa1+fJmcKdZ+RLw6Gu\n"
        "5QVN0nkgN6OEHE7nEfQHYW9+4QUuIVTwSyS+D0+thXJP0RXDUuj/tTGIjmMwTgu1\n"
        "NuXhc3ECgYEA0AZdDL9g0mwxQTzitA3YTAuVM/nvvzy9MqcPZO8uiA7PKIYQ9ip/\n"
        "jfQSVhP2Q6hCyhwKNUUwBxhqQSlNgILdia3OExj95+g+pCfUIhl0bfZGCLEMAH+5\n"
        "fmRT8xhUIqmVdaMLeVFXtmcmhF082zSeq+E8dw1c5OvxuZ4TVkonHrM=\n"
        "-----END RSA PRIVATE KEY-----\n\0";
    publicKey =
        "AAAAB3NzaC1yc2EAAAADAQABAAABAQDECzDGJl8T+YdUjaeF0onV\n"
        "QChMZE1eQTGuzmmtAnusBOIpYz0v6z91/uex9KFm50uanOUFA9TGLfy5xWm8LmGfk\n"
        "P3CEG6TYhVYX7KHcYTHX/3+JlQR2e2s1EYiS7hMYqs4C71pin7qjYcKSrDq/WvKJ7f\n"
        "G+wVtRAGH+eDAbynncGVR5M7mUMIcKaUEr0VNNnA+zjznqxr1UL4Znt8RdbdZEm73iY\n"
        "q4Wu3JYs23rR+7aLOp3rohU3GbWusNs7IQiKd3aFFpQmbdu/"
        "pwKgcuyvRrD08rpUeqtGH\n"
        "1JoqNQfdTScERwBebg59q89C/hK4Z0CKiuYGxxYM1TVX7voD3qGd1\n";
}
void SigstructGenerator::doEcreate(uint64_t size) {
    Ecreate ecreate = {};
    char cmd[] = "ECREATE";
    memcpy(ecreate.cmd, cmd, strlen(cmd));
    ecreate.fst = 1;
    ecreate.size = size;
    memcpy(ecreate.suffix, "", 44);
    SHA256_Update(&c, &ecreate, sizeof(struct Ecreate));
}

void SigstructGenerator::doEadd(uint64_t offset, sec_info_t &sec_info) {
    Eadd eadd = {};
    char cmd[] = "EADD";
    memcpy(eadd.cmd, cmd, strlen(cmd));
    eadd.offset = offset;
    eadd.flags = sec_info.flags;
    memcpy(eadd.suffix, "", 40);
    SHA256_Update(&c, &eadd, sizeof(struct Eadd));
}

void SigstructGenerator::digestFinal() { SHA256_Final(m, &c); }

sigstruct *SigstructGenerator::getSigstruct() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char *signBuffer = (char *)malloc(128 * sizeof(char) * 2);
    char *p = signBuffer;
    char *q1;
    char *q2;
    unsigned char *signature;
    char *modulus;
    RSA *pub = generatePubRSA(publicKey);
    BIGNUM *modulusBN = pub->n;
    BIGNUM *signatureBN;
    BIGNUM *tmp1;
    BIGNUM *tmp2;
    BIGNUM *q1BN;
    BIGNUM *q2BN;
    BIGNUM *q2TMP;
    BIGNUM *rem2;
    BN_CTX *ctx;
    unsigned short year = 1900 + ltm->tm_year;
    unsigned char month = (1 + ltm->tm_mon);
    unsigned char day = ltm->tm_mday;
    uint32_t ymd = year;
    ymd <<= 8;
    ymd += month;
    ymd <<= 8;
    ymd += day;
    sstruct.header11 = 0x00000006000000e1;
    sstruct.header12 = 0x0001000000000000;
    sstruct.vendor = 0x00000000;
    sstruct.date = ymd;
    sstruct.header21 = 0x0000010100000060;
    sstruct.header22 = 0x0000006000000001;
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
    memcpy(p, &sstruct.header11, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.header12, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.vendor, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, &sstruct.date, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, &sstruct.header21, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.header22, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.swdefined, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, sstruct.reserved1, sizeof(uint8_t) * 84);
    p += sizeof(uint8_t) * 84;
    memcpy(p, &sstruct.miscselect, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, &sstruct.micsmask, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, &sstruct.reserved2, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, &sstruct.isvfamilyid1, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.isvfamilyid2, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.attributes1, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.attributes2, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.attributemask1, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.attributemask2, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, sstruct.enclavehash, sizeof(char) * 32);
    p += (sizeof(char) * 32);
    memcpy(p, sstruct.reserved3, sizeof(uint8_t) * 16);
    p += sizeof(uint8_t) * 16;
    memcpy(p, &sstruct.isvextprodid1, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.isvextprodid2, sizeof(uint64_t));
    p += sizeof(uint64_t);
    memcpy(p, &sstruct.isvprodid, sizeof(uint16_t));
    p += sizeof(uint16_t);
    memcpy(p, &sstruct.isvsvn, sizeof(uint16_t));
    p += sizeof(uint16_t);
    memcpy(p, sstruct.reserved4, sizeof(uint8_t) * 12);
    string plainTxt(signBuffer);
    signMsg(plainTxt, signature);
    BN_hex2bn(&signatureBN, signature);
    BN_add(tmp1, signatureBN, modulusBN);
    BN_div(q1BN, tmp2, tmp1, modulusBN, ctx);
    BN_mul(q2TMP, tmp2, signatureBN, ctx);
    BN_div(q2BN, rem2, q2TMP, modulusBN, ctx);
    q1 = BN_bn2hex(q1BN);
    q2 = BN_bn2hex(q2BN);
    modulus = BN_bn2hex(modulusBN);
    memcpy(sstruct.modulus, modulus, sizeof(uint8_t) * 384);
    sstruct.exponent = 3;
    memcpy(sstruct.signature, signature, sizeof(uint8_t) * 384);
    memcpy(sstruct.q1, q1, sizeof(uint8_t) * 384);
    memcpy(sstruct.q2, q2, sizeof(uint8_t) * 384);
    free(signBuffer);
    free(signature);
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
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
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

void SigstructGenerator::signMsg(string plainText, unsigned char *encMsg) {
    RSA *privateRSA = generatePriRSA(privateKey);
    size_t encMessageLength;
    RSASign(privateRSA, (unsigned char *)plainText.c_str(), plainText.length(),
            &encMsg, &encMessageLength);
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

string TokenGetter::getToken(const sigstruct *sig) {
    string ret;

    aesm::message::Request request;
    request.mutable_getlictokenreq()->set_mr_enclave(sig->signature, 384);
    request.mutable_getlictokenreq()->set_mr_signer(sig->modulus);
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
            write(this->sockfd, sendBuf.data(), sendBuf.length() - beginInd);
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
    while (beginInd < recvLen) {
        ssize_t nbytes =
            read(this->sockfd, recvBuf + beginInd, recvLen - beginInd);
        if (nbytes < 0) {
            console->error("Cannot write to aems socket {}", strerror(errno));
            exit(-1);
        }
        beginInd += nbytes;
    }

    aesm::message::Response response;
    response.ParseFromArray(recvBuf, recvLen);
    console->debug("Dump protobuf recv message: {}", response.DebugString());

    auto err = response.getlictokenres().errorcode();
    if (err) {
        console->error("Cannot get launch token: {}", err);
        exit(-1);
    }

    ret = response.getlictokenres().token();
    return ret;
}
