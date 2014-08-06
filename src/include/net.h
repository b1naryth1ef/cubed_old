#pragma once

#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

class Client {
    private:
        std::vector<Channel *> channels;
        int ep_fd;
    public:
        Client();
};

class Channel {
    public:
        bool connect(std::string addr, int port);
        bool disconnect(int reason);
        int getDescriptor();
};

class UDPChannel: public Channel {

};

class TCPChannel: public Channel {

};