#pragma once

#include "global.h"

#include "flatbuffers/flatbuffers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

// generated Files
#include "gen/packet_generated.h"
#include "gen/handshake_generated.h"

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
        bool active;
        bool open(std::string addr, int port);
        bool close(int reason);
        int getFD() { return fd; }
};

class UDPService: public Service {
    public:
        std::thread read_loop_thread;

        bool open(std::string addr, int port);
        bool close(int reason);
        void read_loop();
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

// ServiceClient represent remote clients on a local service
class ServiceClient {
    private:
        char buffer[4096];
};

class UDPServiceClient {};
class TCPServiceClient {};

class NetServer {

};


class Packet {
    public:
        NetClient *cli;
        Channel *chan;
};