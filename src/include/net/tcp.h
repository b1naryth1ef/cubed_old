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
class TCPServer;
class TCPConnection;

enum TCPEventType {
    TCP_CONNECT,
    TCP_DISCONNECT,
    TCP_MESSAGE,
};

class TCPEvent {
    public:
        TCPEventType type;

        TCPServer *server;
        TCPServerClient *client;
        TCPConnection *conn;

        muduo::net::Buffer *buffer;
        muduo::Timestamp timestamp;

        TCPEvent(TCPEventType type) {
            this->type = type;
        }

        TCPEvent& setServer(TCPServer *server) {
            this->server = server;
            return *this;
        }

        TCPEvent& setClient(TCPServerClient *client) {
            this->client = client;
            return *this;
        }

        TCPEvent& setConn(TCPConnection *conn) {
            this->conn = conn;
            return *this;
        }

        TCPEvent& setBuffer(muduo::net::Buffer *buffer) {
            this->buffer = buffer;
            return *this;
        }

        TCPEvent& setTimestamp(muduo::Timestamp ts) {
            this->timestamp = ts;
            return *this;
        }
};

typedef std::function<void (TCPEvent&)> TCPHook;

class TCPEventDispatcher {
    private:
        TCPHook onEvent;

    public:
        void addEventCallback(TCPHook callback) {
            this->onEvent = callback;
        }

        void triggerEvent(TCPEvent &event) {
            if (this->onEvent != nullptr) {
                this->onEvent(event);
            }
        }

        bool hasEventHandler() {
            return (onEvent != nullptr);
        }
};



class TCPServer : public TCPEventDispatcher {
    private:
        muduo::net::EventLoop *loop;
        muduo::net::TcpServer *server;

        std::set<TCPServerClient *> clients;

    public:
        TCPServer(muduo::net::EventLoop*, ConnString);
        ~TCPServer();

        void start();

        void onNewConnection(const muduo::net::TcpConnectionPtr&);

        void addClient(TCPServerClient*);
        void rmvClient(TCPServerClient*);
        TCPServerClient *getClient(const muduo::net::TcpConnectionPtr&);
};

class TCPServerClient : public TCPEventDispatcher {
    private:
        TCPServer *server;

    public:
        const muduo::net::TcpConnectionPtr &conn;

        TCPServerClient(TCPServer*, const muduo::net::TcpConnectionPtr&);

        void onMessage(
            const muduo::net::TcpConnectionPtr&,
            muduo::net::Buffer*,
            muduo::Timestamp);

        void send(std::string);
};

class TCPConnection : public TCPEventDispatcher {
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

        void send(std::string);
};

}

