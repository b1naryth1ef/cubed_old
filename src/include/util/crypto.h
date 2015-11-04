#pragma once

#include "global.h"
#include "ioutil.h"
#include <sodium.h>
#include <algorithm>
#include <stdexcept>

#include "packet.pb.h"

static std::string sha512(std::string data) {
    unsigned char out[crypto_hash_sha512_BYTES];
    crypto_hash_sha512_state state;

    crypto_hash_sha512_init(&state);
    crypto_hash_sha512_update(&state,
        (const unsigned char *) data.c_str(),
        data.size());
    crypto_hash_sha512_final(&state, out);

    return std::string((char *) out, crypto_hash_sha512_BYTES);
}

static std::string hex_to_string(const std::string& input) {
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();
    if (len & 1) throw std::invalid_argument("odd length");

    std::string output;
    output.reserve(len / 2);
    for (size_t i = 0; i < len; i += 2)
    {
        char a = input[i];
        const char* p = std::lower_bound(lut, lut + 16, a);
        if (*p != a) throw std::invalid_argument("not a hex digit");

        char b = input[i + 1];
        const char* q = std::lower_bound(lut, lut + 16, b);
        if (*q != b) throw std::invalid_argument("not a hex digit");

        output.push_back(((p - lut) << 4) | (q - lut));
    }
    return output;
}

static std::string string_to_hex(const std::string& input) {
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }

    std::transform(output.begin(), output.end(), output.begin(), ::tolower);
    return output;
}

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

        std::string newNonce() {
            unsigned char nonce[crypto_box_NONCEBYTES];
            randombytes_buf(nonce, sizeof nonce);
            return std::string((char *) nonce, crypto_box_NONCEBYTES);
        }

        std::string encrypt(const std::string &data, std::string nonce, KeyPair other) {
            return this->encrypt(data, nonce, other.getPublicKey());
        }

        std::string encrypt(const std::string &data, std::string nonce, std::string other) {
            unsigned char encrypted[crypto_box_MACBYTES + data.size()];

            int res = crypto_box_easy(encrypted,
                (const unsigned char *) data.c_str(), data.size(),
                (const unsigned char *) nonce.c_str(),
                (const unsigned char *) other.c_str(),
                (const unsigned char *) this->privkey.c_str()
            );

            if (res != 0) {
                throw Exception("Failed to encrypt data!");
            }

            return std::string((char *) encrypted, crypto_box_MACBYTES + data.size());
        }

        std::string decrypt(const std::string &data, std::string nonce, KeyPair other) {
            size_t cleartext_len = data.size() - crypto_box_MACBYTES;
            unsigned char cleartext[cleartext_len];

            int res =crypto_box_open_easy(
                cleartext,
                (const unsigned char *) data.c_str(), data.size(),
                (const unsigned char *) nonce.c_str(),
                (const unsigned char *) other.getPublicKey().c_str(),
                (const unsigned char *) this->privkey.c_str()
            );

            if (res != 0) {
                throw Exception("Failed to decrypt data!");
            }
            return std::string((char *) cleartext, cleartext_len);
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

        KeyPair() {}

        void loadFromString(std::string priv, std::string pub) {
            this->privkey = priv;
            this->pubkey = pub;
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
