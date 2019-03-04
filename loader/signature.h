#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <sgx_arch.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <string>

using namespace std;

struct Ecreate {
    char cmd[8];
    uint32_t fst;
    uint64_t size;
    char suffix[44];
}  __attribute__((packed));

struct Eadd {
    char cmd[8];
    uint64_t offset;
    uint64_t flags;
    char suffix[40];
}  __attribute__((packed)) ;


class SigstructGenerator {
private:
    secs_t * scs;
    sigstruct sstruct;
    SHA256_CTX c;
    unsigned char m[SHA256_DIGEST_LENGTH];
    string privateKey;
    string publicKey;

    RSA *generatePriRSA(string key);
    RSA *generatePubRSA(string key);
    int RSASign(RSA *rsa, const unsigned char *Msg,
        size_t MsgLen, unsigned char** EncMsg, size_t* EncMsgLen);
    //void signMsg(string plainText, unsigned char *encMsg);
    string signMsg(string text);
public:
    SigstructGenerator(secs_t *secs);
    void doEcreate(uint64_t size, uint32_t nframes);
    void doEadd(uint64_t offset, sec_info_t &sec_info);
    void digestFinal();
    sigstruct *getSigstruct();

    void readPrivateKeyFile(const string &filename);
    void readPublicKeyFile(const string &filename);
};

class TokenGetter {
private:
    int sockfd;
    einittoken_t token = {};

    void dumpToken();
public:
    TokenGetter(const string &filename);
    ~TokenGetter();
    const einittoken_t *getToken(const sigstruct *sig);
};

#endif
