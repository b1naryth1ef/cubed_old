#pragma once

#include "http.h"

class LoginServer {
    public:
        std::string url;
        bool valid;

        LoginServer(std::string u) {
            this->url = u;
            this->valid = this->verify();
        }

        bool verify() {
            HTTPClient cli;

            char buffer[this->url.size() + 10];
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
};
