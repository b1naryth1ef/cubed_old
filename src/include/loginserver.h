#pragma once

#include "global.h"
#include "http.h"
#include "crypto.h"
#include "base64.h"

class LoginServer {
    public:
        std::string url;
        bool valid;

        LoginServer(std::string u) {
            this->url = u;
            this->valid = this->verify();
        }

        /*
        Attempts to calculate a proof-of-work response based on a challenge
        from the server. This is used to reduce the risk of DDoS/spam attacks
        against login servers.
        */
        std::string proveWork() {
            HTTPClient cli;

            // First, lets get a proof-of-work "challenge"
            char buffer[this->url.size() + 9];
            sprintf(buffer, "%s/api/hash", this->url.c_str());
            HTTPRequest hashreq(buffer);
            HTTPResponse hashres = cli.request(hashreq);

            // Parse the JSON challenge response
            rapidjson::Document d;
            hashres.asJSON(&d);

            // Load represents how much work we're required to do
            int load = d["load"].GetInt();
            DEBUG("Got POW response, workload is %i", load);

            // We're never going to guess this, might even be a malicious
            //  login server.
            if (load > 1073741824) {
                WARN("The load for the POW challenge is wayyyyy to high!");
            }

            // Load the base and hash attributes
            std::string baseorg = d["base"].GetString();
            std::string base = base64_decode(baseorg);
            std::string hash = d["hash"].GetString();

            // Our guess will start at 0, and grow until it hits load
            int guess = 0;
            bool found = false;
            char guess_buffer[128];

            // We'll guess until we hit the answer or fail
            while (guess <= load) {
                sprintf(guess_buffer, "%i%s", guess, base.c_str());
                std::string guess_hash = string_to_hex(sha512(guess_buffer));
                if (guess_hash == hash) {
                    found = true;
                    break;
                }
                guess++;
            }

            // If we didn't find the answer, throw an error
            if (!found) {
                ERROR("Failed to calculate proof of work!");
                throw Exception("Failed to calculate proof of work!");
            }

            char *base_f = curl_easy_escape(cli.c, baseorg.c_str(), baseorg.size());

            char urlbuffer[128];
            sprintf(urlbuffer, "powq=%s&powa=%i",
                base_f, guess);
            curl_free(base_f);
            return std::string(urlbuffer);
        }

        bool verify() {
            HTTPClient cli;

            char buffer[this->url.size() + 9];
            sprintf(buffer, "%s/api/info", this->url.c_str());

            HTTPResponse r = cli.request(HTTPRequest(buffer));

            if (r.code != 200) {
                ERROR("Failed to verify login server %s", this->url.c_str());
                return false;
            } else {
                INFO("Verified login server %s", this->url.c_str());
            }

            return true;
        }

        bool createAccount(KeyPair *kp) {
            HTTPClient cli;

            // Obtain a proof-of-work response
            std::string proof = this->proveWork();
        
            // Build the URL
            char buffer[this->url.size() + 128];
            sprintf(buffer, "%s/api/register?%s",
                this->url.c_str(),
                proof.c_str());

            // URL escape the url
            DEBUG("Buffer: %s", buffer);
            HTTPRequest req(buffer);
            req.setMethod("POST");

            HTTPResponse r = cli.request(req);

            if (r.code != 200) {
                ERROR("Failed to create an account %s", this->url.c_str());
                return false;
            }

            rapidjson::Document d;
            r.asJSON(&d);

            DEBUG("Created account!");
        }

        std::string login(KeyPair &kp) {}
};
