#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <sgx_arch.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <string>

struct Ecreate {
    char cmd[8];
    uint32_t fst;
    uint64_t size;
    char suffix[44];
};

struct Eadd {
    char cmd[8];
    uint64_t offset;
    uint64_t flags;
    char suffix[40];
};


class SigstructGenerator {
private:
    secs_t * scs;
    sigstruct sstruct;
    SHA256_CTX c;
    unsigned char m[SHA256_DIGEST_LENGTH];
    string privateKey;
    string publicKey;
public:
    SigstructGenerator(secs_t *secs);
    void doEcreate(uint64_t size);
    void doEadd(uint64_t offset, sec_info_t &sec_info);
    void digestFinal();
    sigstruct *getSigstruct();
    RSA *generatePriRSA(string key);
    int RSASign(RSA *rsa, const unsigned char *Msg,\
    size_t MsgLen, unsigned char** EncMsg, size_t* EncMsgLen);
    void base64Encode(unsigned char* buffer,\
    size_t length, char** base64Text);
    char* signMsg(string plainText);
    void finalize();
};

#endif
