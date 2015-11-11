#pragma once

#include <netinet/in.h>
#include "uv.h"

#include "global.h"
#include "util/util.h"

#include <muduo/net/InetAddress.h>

namespace Net {

/**
ConnString represents a struct containing a host:port mapping.
**/
struct ConnString {
    std::string host;
    unsigned short port;

    ConnString(std::string conns, unsigned short port) {
        this->host = conns;
        this->port = port;
    }

    ConnString(std::string conns) {
        if (conns.find(':') != -1) {
            auto parts = splitString(conns, ':');
            if (parts.size() == 2) {
                this->host = parts[0];

                std::string::size_type sz;
                this->port = std::stoi(parts[1], &sz);

                return;
            }
        }

        ERROR("Invalid ConnectionInfo string: %s", conns.c_str());
        throw Exception("Invalid ConnectionInfo string");
    }

    std::string toString() {
        char buffer[512];
        sprintf(&buffer[0], "%s:%i", this->host.c_str(), this->port);
        return std::string(buffer);
    }
};

}
