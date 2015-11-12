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
    if (!this->hasEventHandler()) {
        throw Exception("Cannot start TCPServer without an onEvent handler");
    }

    this->server->start();
}

void TCPServer::onNewConnection(const muduo::net::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        DEBUG("TCPServer recieved new connection, passing to event handler");

        TCPServerClient* client = new TCPServerClient(this, conn);
        this->addClient(client);

        this->triggerEvent(TCPEvent(TCP_CONNECT).setClient(client));
    } else {
        DEBUG("TCPServer lost connection, passing to event handler");
        TCPServerClient* client = this->getClient(conn);

        this->rmvClient(client);

        this->triggerEvent(TCPEvent(TCP_DISCONNECT).setClient(client));
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

TCPServerClient::TCPServerClient(TCPServer *server, const muduo::net::TcpConnectionPtr &cn) : conn(cn) {
    this->server = server;
    conn->setMessageCallback(std::bind(&TCPServerClient::onMessage, this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3));
}

void TCPServerClient::onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buff, muduo::Timestamp ts) {
    TCPEvent event = TCPEvent(TCP_MESSAGE).setClient(this).setServer(this->server).setBuffer(buff).setTimestamp(ts);
    this->triggerEvent(event);
    this->server->triggerEvent(event);
}

void TCPServerClient::send(std::string data) {
    conn->send(data);
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
    if (conn->connected()) {
        this->triggerEvent(TCPEvent(TCP_CONNECT).setConn(this));
    } else {
        this->triggerEvent(TCPEvent(TCP_DISCONNECT).setConn(this));
    }
    DEBUG("Established connection");
}

void TCPConnection::onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buff, muduo::Timestamp ts) {
    this->triggerEvent(TCPEvent(TCP_MESSAGE).setConn(this).setBuffer(buff).setTimestamp(ts));
}

void TCPConnection::connect() {
    this->client->connect();
}

void TCPConnection::send(std::string data) {
    const muduo::net::TcpConnectionPtr ptr = this->client->connection();
    // muduo::string buff(data);
    ptr->send(data);
}

}
