#pragma once

#include <netinet/in.h>

#include "global.h"
#include "util/util.h"
#include "net/util.h"

#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>


namespace Net {

class TCPServerClient;

enum TCPEventType {
    TCP_CONNECT,
    TCP_DISCONNECT,
    TCP_MESSAGE,
};

class TCPEvent {
    public:
        TCPEventType type;
        TCPServerClient *client;

        muduo::net::Buffer *buffer;
        muduo::Timestamp timestamp;

        TCPEvent(TCPEventType type, TCPServerClient *client) {
            this->type = type;
            this->client = client;
        }

        TCPEvent(TCPEventType type, TCPServerClient *client, muduo::net::Buffer *buffer, muduo::Timestamp ts) {
            this->type = type;
            this->client = client;
            this->buffer = buffer;
            this->timestamp = ts;
        }
};


typedef std::function<void (TCPEvent*)> TCPServerHook;

class TCPServer {
    private:
        muduo::net::EventLoop *loop;
        muduo::net::TcpServer *server;

        std::set<TCPServerClient *> clients;

        TCPServerHook onEvent;

    public:
        TCPServer(muduo::net::EventLoop*, ConnString);
        ~TCPServer();

        void start();

        void onNewConnection(const muduo::net::TcpConnectionPtr&);

        void addClient(TCPServerClient*);
        void rmvClient(TCPServerClient*);
        TCPServerClient *getClient(const muduo::net::TcpConnectionPtr&);

        void addEventCallback(TCPServerHook callback);
        void triggerEvent(TCPEvent*);
};

class TCPServerClient {
    private:
        TCPServer *server;

    public:
        const muduo::net::TcpConnectionPtr &conn;

        TCPServerClient(TCPServer*, const muduo::net::TcpConnectionPtr&);

        void onMessage(
            const muduo::net::TcpConnectionPtr&,
            muduo::net::Buffer*,
            muduo::Timestamp);
};

class TCPConnection {
    private:
        muduo::net::EventLoop *loop;
        muduo::net::TcpClient *client;

        ConnString &remote;
    public:
        TCPConnection(muduo::net::EventLoop*, ConnString);

        void onNewConnection(const muduo::net::TcpConnectionPtr&);
        void onMessage(
            const muduo::net::TcpConnectionPtr&,
            muduo::net::Buffer*,
            muduo::Timestamp);

        void connect();
};

}

