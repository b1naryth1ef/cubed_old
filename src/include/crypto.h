#pragma once

#include "global.h"
#include "ioutil.h"
#include <sodium.h>


class KeyPair {
    private:
        unsigned char *pubkey;
        unsigned char *privkey;

    public:
        bool empty = true;

        std::string getPublicKey() {
            return std::string(reinterpret_cast<const char*>(this->pubkey));
        }

        // TODO: add support for detached mode
        std::string sign(std::string data) {
            long long unsigned int buffer_size = crypto_sign_BYTES + (long long) data.size();
            unsigned char buffer[buffer_size];

            assert(crypto_sign(
                buffer, &buffer_size,
                (const unsigned char *) data.c_str(), data.size(),
                this->privkey
            ) == 0);

            return std::string(reinterpret_cast<const char*>(buffer));
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

            this->pubkey = new unsigned char[crypto_box_PUBLICKEYBYTES];
            this->privkey = new unsigned char[crypto_box_SECRETKEYBYTES];

            std::ifstream privf(privpath.c_str());
            std::ifstream pubf(pubpath.c_str());

            privf.read((char *) this->privkey, 32);
            pubf.read((char *) this->pubkey, 32);

            this->empty = false;
        }

        ~KeyPair() {
            free(this->pubkey);
            free(this->privkey);
        }

        void generate() {
            if (!this->empty) {
                throw Exception("Cannot generate on a filled keyPair!");
            }

            this->pubkey = new unsigned char[crypto_box_PUBLICKEYBYTES];
            this->privkey = new unsigned char[crypto_box_SECRETKEYBYTES];
            crypto_box_keypair(this->pubkey, this->privkey);
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
            file.write((char*)this->privkey, crypto_box_SECRETKEYBYTES);
            file.close();

            // Write public
            file.open(pubpath, std::ios::out | std::ios::binary);
            file.write((char*)this->pubkey, crypto_box_PUBLICKEYBYTES);
            file.close();
        }
};

