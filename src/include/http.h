#pragma once

#include <string>
#include <curl/curl.h>

static size_t util_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    std::string *buff = (std::string *) userp;
    size_t realsize = size * nmemb;
    buff->append((const char *) contents, realsize);
    return realsize;
}

class HTTPResponse {
    public:
        long code;
        std::string data;
};

class HTTPRequest {
    public:
        std::string url;
        std::string meth = "GET";

        HTTPRequest(std::string url) {
            this->url = url;
        }

        void setMethod(std::string m) {
            this->meth = m;
        }

        void build(CURL *c, std::string *res) {
            curl_easy_setopt(c, CURLOPT_URL, url.c_str());
            curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, meth.c_str());
            curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, util_write_callback);
            curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *) res);
        }
};

class HTTPClient {
    public:
        CURL *c;

        HTTPClient() {
            this->c = curl_easy_init();
        }

        ~HTTPClient() {
            curl_easy_cleanup(this->c);
        }

        HTTPResponse request(HTTPRequest r) {
            HTTPResponse x;
            r.build(this->c, &x.data);
            curl_easy_perform(this->c);
            curl_easy_getinfo(this->c, CURLINFO_RESPONSE_CODE, &x.code);
            return x;
        }
};