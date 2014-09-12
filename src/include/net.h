#pragma once

#include "global.h"
#include "crypto.h"

// generated Files
#include "gen/packet.pb.h"

// Networking specific headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/epoll.h>

enum PacketType {
    PACKET_STATUS_REQUEST,
    PACKET_STATUS_RESPONSE,
    PACKET_HANDSHAKE,
    PACKET_INIT,
    PACKET_DC,
};

class Packet {};

#define MAXEVENTS 128
#define STATUS_JUNK_DATA_SIZE 256

static int openTCPSocket(std::string, short);

class RemoteClient;

class TCPRemoteClient {
    public:
        std::string host;
        ushort port;
        ushort id;
        int fd;

        std::vector<char> buffer;

        RemoteClient *remote;

        TCPRemoteClient(int f, std::string h, ushort p) {
            this->fd = f;
            this->host = h;
            this->port = p;
        }

        ~TCPRemoteClient() {
            if (this->fd) close(this->fd);
        }

        std::string toString() {
            char fmt[512];
            sprintf(fmt, "TCPClient<%s, %i, %i>", this->host.c_str(), this->port, this->fd);
            return std::string(fmt);
        }

        void disconnect() {
            close(this->fd);
        }

        void send_packet(int id, google::protobuf::Message *data);
};

typedef std::function<bool (TCPRemoteClient *)> TCPServerHook;

class TCPServer {
    public:
        KeyPair *kp;
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

        TCPServer(std::string, ushort);
        ~TCPServer();

        void stop();
        void loop();
        void closeRemote(int);
        void processEvent(int);

        bool makeNonBlocking(int);
};

typedef std::function<bool (void)> TCPClientHook;

class TCPClient {
    public:
        KeyPair *our_kp;
        KeyPair *serv_kp;

        TCPClient(std::string, ushort);
        ~TCPClient();

        std::string remote_host;
        ushort remote_port;
        int fd;
        bool active = true;

        std::thread read_loop_thread;
        std::vector<char> buffer;

        TCPClientHook onConnectionData;
        TCPClientHook onConnectionClose;
        TCPClientHook onConnectionOpen;

        bool conn();
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

        // Clients keypairs
        KeyPair *our_kp;

        // Severs keypair
        KeyPair *serv_kp;

        void tryParse();
        void disconnect(int, const std::string);
        void terminate();
};