#pragma once

#include "global.h"
#include "ioutil.h"
#include <sodium.h>

#include "gen/packet.pb.h"

class SignedData {
    public:
        std::string data;
        std::string sign;

        SignedData(std::string a, std::string b) {
            this->data = a;
            this->sign = b;
        }

        SignedData(cubednet::SignedMessage m) {
            this->data = m.data();
            this->sign = m.signature();
        }

        void toPacket(cubednet::SignedMessage *m) {
            m->set_data(this->data);
            m->set_signature(this->sign);
        }
};

class KeyPair {
    private:
        std::string pubkey;
        std::string privkey;

    public:
        bool empty = true;

        std::string getPublicKey() {
            return this->pubkey;
        }

        SignedData sign(const std::string &data) {
            size_t datalen = data.size();

            unsigned char m[datalen];
            unsigned char signature[crypto_sign_BYTES];
            unsigned long long siglen;

            for (int i=0; i < datalen; i++) m[i] = data[i];
            crypto_sign(
                signature, &siglen,
                m, datalen,
                (const unsigned char *) this->privkey.c_str()
            );

            return SignedData(data, std::string((char *) signature, siglen));
        }

        bool validate(SignedData d) {
            int res = crypto_sign_verify_detached(
                (const unsigned char *) d.sign.c_str(),
                (const unsigned char *) d.data.c_str(),
                (unsigned long long) d.data.size(),
                (const unsigned char *) this->pubkey.c_str()
            );

            if (res != 0) {
                return false;
            } else {
                return true;
            }
        }

        KeyPair(std::string dir) {
            if (!ioutil::file_exists(dir)) {
                return;
            }

            std::string privpath = ioutil::join(dir, "private.key");
            std::string pubpath = ioutil::join(dir, "public.key");

            if (!ioutil::file_exists(privpath) || !ioutil::file_exists(pubpath)) {
                return;
            }

            std::ifstream privf(privpath.c_str());
            std::ifstream pubf(pubpath.c_str());

            privf.read(&this->privkey[0], crypto_sign_SECRETKEYBYTES);
            pubf.read(&this->pubkey[0], crypto_sign_PUBLICKEYBYTES);

            this->empty = false;
        }

        void generate() {
            if (!this->empty) {
                throw Exception("Cannot generate on a filled keyPair!");
            }

            unsigned char pk[crypto_sign_PUBLICKEYBYTES];
            unsigned char sk[crypto_sign_SECRETKEYBYTES];

            crypto_sign_keypair(pk, sk);
            this->privkey = std::string((char *) sk, sizeof sk);
            this->pubkey = std::string((char *) pk, sizeof pk);
            this->empty = false;
        }

        void save(std::string dir) {
            std::string privpath, pubpath;

            privpath = ioutil::join(dir, "private.key");
            pubpath = ioutil::join(dir, "public.key");

            if (ioutil::file_exists(privpath)) {
                remove(privpath.c_str());
            }

            if (ioutil::file_exists(pubpath)) {
                remove(pubpath.c_str());
            }

            if (!ioutil::file_exists(dir)) {
                mkdir(dir.c_str(), 0777);
            }

            std::ofstream file;

            // Write private
            file.open(privpath, std::ios::out | std::ios::binary);
            file.write(this->privkey.c_str(), crypto_sign_SECRETKEYBYTES);
            file.close();

            // Write public
            file.open(pubpath, std::ios::out | std::ios::binary);
            file.write(this->pubkey.c_str(), crypto_sign_PUBLICKEYBYTES);
            file.close();
        }
};

