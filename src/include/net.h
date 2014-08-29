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

#define MAXEVENTS 64

class TCPServer {
    public:
        struct sockaddr_in serv_addr;

        int sockfd, efd;
        int backlog = 64;
        bool active = true;

        std::thread loop_thread;

        TCPServer(std::string, short);
        ~TCPServer();

        void stop();
        void loop();

        bool makeNonBlocking(int);
};

