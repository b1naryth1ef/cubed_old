#pragma once

#include "global.h"

#include "flatbuffers/flatbuffers.h"

// generated Files
#include "gen/packet_generated.h"
#include "gen/handshake_generated.h"

// TODO: cut down
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>

class Packet {};

#define MAXEVENTS 128

static int openTCPSocket(std::string, short);

class TCPClient {
    public:
        std::string host;
        short port;
        int fd;

        TCPClient(int f, std::string h, short p) {
            this->fd = f;
            this->host = h;
            this->port = abs(p);
        }

        ~TCPClient() {
            close(this->fd);
        }

        std::string toString() {
            char fmt[512];
            sprintf(fmt, "TCPClient<%s, %i, %i>", this->host.c_str(), this->port, this->fd);
            return std::string(fmt);
        }
};

class TCPServer {
    public:
        std::map<int, TCPClient*> clients;

        int sfd, efd;
        int backlog = 128;
        bool active = true;

        struct epoll_event event;
        struct epoll_event *events;

        std::thread loop_thread;

        TCPServer(std::string, short);
        ~TCPServer();

        void stop();
        void loop();
        void processEvent(int i);

        bool makeNonBlocking(int);
};

