#pragma once

#include "global.h"

// #include "flatbuffers/flatbuffers.h"

// generated Files
#include "gen/packet.pb.h"

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

enum PacketType {
    PACKET_HELLO,
    PACKET_DC,
};

class Packet {};

#define MAXEVENTS 128

static int openTCPSocket(std::string, short);

class TCPRemoteClient {
    public:
        std::string host;
        ushort port;
        ushort id;
        int fd;

        std::vector<char> buffer;

        TCPRemoteClient(int f, std::string h, short p) {
            this->fd = f;
            this->host = h;
            this->port = abs(p);
        }

        ~TCPRemoteClient() {
            close(this->fd);
        }

        std::string toString() {
            char fmt[512];
            sprintf(fmt, "TCPClient<%s, %i, %i>", this->host.c_str(), this->port, this->fd);
            return std::string(fmt);
        }

        void send_packet(int id, google::protobuf::Message *data);
};

typedef std::function<bool (TCPRemoteClient *)> TCPServerHook;

class TCPServer {
    public:
        std::map<int, TCPRemoteClient*> clients;

        int sfd, efd;
        int backlog = 128;
        bool active = true;

        struct epoll_event event;
        struct epoll_event *events;

        std::thread loop_thread;

        TCPServerHook onConnectionOpen;
        TCPServerHook onConnectionClose;
        TCPServerHook onConnectionData;

        TCPServer(std::string, short);
        ~TCPServer();

        void stop();
        void loop();
        void closeRemote(int);
        void processEvent(int);

        bool makeNonBlocking(int);
};

class TCPClient {
    public:
        ~TCPClient();

        std::string remote_host;
        ushort remote_port;
        int fd;
        bool active = true;

        std::thread read_loop_thread;

        std::vector<char> buffer;

        bool conn(std::string, ushort);
        void read_loop();
        void send_packet(int, google::protobuf::Message*);
};

enum RemoteClientState {
    STATE_NEW,
    STATE_HANDSHAKE,
    STATE_CONNECTED,
    STATE_QUITTING,
    STATE_DEAD
};

class RemoteClient {
    public:
        ushort id;
        RemoteClientState state;
        TCPRemoteClient *tcp;

        std::queue<cubednet::Packet*> packet_buffer;

        void tryParse();
        void disconnect(int, const std::string);
};