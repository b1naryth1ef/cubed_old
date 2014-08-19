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

class Channel {
    public:
        bool connect(std::string addr, int port);
        bool disconnect(int reason);
        int getFD();
};

class UDPChannel: public Channel {

};

class TCPChannel: public Channel {

};

class Service {
    public:
        int fd;
        bool open(std::string addr, int port);
        bool close(int reason);
        int getFD() { return fd; }
};

class UDPService: public Service {
    public:
        bool open(std::string addr, int port);
        bool close(int reason);
};

class TCPService: public Service {
    public:
        bool open(std::string addr, int port);
        bool close(int reason);

};

class NetClient {
    private:
        std::vector<Channel *> channels;
        int ep_fd;
    public:
        NetClient();
};

class NetServer {

};


class Packet {
    public:
        NetClient *cli;
        Channel *chan;
};