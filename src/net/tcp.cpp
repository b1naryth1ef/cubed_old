#include "net/tcp.h"

namespace Net {

TCPServer::TCPServer(muduo::net::EventLoop *loop, ConnString cs) {
    this->loop = loop;

    const muduo::net::InetAddress &addr = muduo::net::InetAddress(cs.host, cs.port);

    INFO("TCPServer binding on %s:%i", cs.host.c_str(), cs.port);

    this->server = new muduo::net::TcpServer(loop, addr, "");
    this->server->setConnectionCallback(std::bind(&TCPServer::onNewConnection, this, std::placeholders::_1));
}

TCPServer::~TCPServer() {}

void TCPServer::start() {
    if (!this->onEvent) {
        throw Exception("Cannot start TCPServer without an onEvent handler");
    }

    this->server->start();
}

void TCPServer::onNewConnection(const muduo::net::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        DEBUG("TCPServer recieved new connection, passing to event handler");

        TCPServerClient* client = new TCPServerClient(this, conn);
        this->addClient(client);

        this->triggerEvent(new TCPEvent(
            TCP_CONNECT,
            client
        ));
    } else {
        DEBUG("TCPServer lost connection, passing to event handler");
        TCPServerClient* client = this->getClient(conn);

        this->rmvClient(client);

        this->triggerEvent(new TCPEvent(
            TCP_DISCONNECT,
            client
        ));

        delete(client);
    }
}

void TCPServer::addClient(TCPServerClient *client) {
    if (clients.find(client) != clients.end()) {
        ERROR("Someone tried to readd client %p to our TCPServer", client);
        throw Exception("Cannot readd TCPServerClient to TCPServer");
    }
    clients.insert(client);
}

void TCPServer::rmvClient(TCPServerClient *client) {
    // TODO: error when not there
    clients.erase(client);
}

TCPServerClient *TCPServer::getClient(const muduo::net::TcpConnectionPtr &cn) {
    for (auto conn : this->clients) {
        if (conn->conn == cn) {
            return conn;
        }
    }
    return nullptr;
}

void TCPServer::addEventCallback(TCPServerHook callback) {
    this->onEvent = callback;
}

void TCPServer::triggerEvent(TCPEvent *event) {
    this->onEvent(event);
    delete(event);
}

TCPServerClient::TCPServerClient(TCPServer *server, const muduo::net::TcpConnectionPtr &cn) : conn(cn) {
    this->server = server;
    conn->setMessageCallback(std::bind(&TCPServerClient::onMessage, this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3));
}

void TCPServerClient::onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buff, muduo::Timestamp ts) {
    this->server->triggerEvent(new TCPEvent(
        TCP_MESSAGE,
        this->server->getClient(conn),
        buff,
        ts
    ));
    muduo::string msg(buff->retrieveAllAsString());
    INFO("NEW MESSAGE %s", msg.c_str());
}

TCPConnection::TCPConnection(muduo::net::EventLoop *loop, ConnString cs) : remote(cs) {
    this->loop = loop;

    const muduo::net::InetAddress &addr = muduo::net::InetAddress(cs.host, cs.port);
    this->client = new muduo::net::TcpClient(loop, addr, "");
    this->client->setConnectionCallback(std::bind(&TCPConnection::onNewConnection, this, std::placeholders::_1));
    this->client->setMessageCallback(std::bind(&TCPConnection::onMessage, this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3));
}

void TCPConnection::onNewConnection(const muduo::net::TcpConnectionPtr &conn) {
    DEBUG("Established connection");
}

void TCPConnection::onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buff, muduo::Timestamp ts) {
    muduo::string msg(buff->retrieveAllAsString());
    DEBUG("Message %s", msg.c_str());
}

void TCPConnection::connect() {
    this->client->connect();
}

}
