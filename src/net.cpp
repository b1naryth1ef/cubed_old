#include "net.h"

static int openTCPSocket(std::string host, short port) {
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *) &serv_addr, sizeof(serv_addr));

    // Set serv_addr port and host
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr);

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        return -1;
    }

    return sockfd;
}

TCPServer::TCPServer(std::string host, ushort port) {
    this->sfd = openTCPSocket(host, port);
    if (this->sfd == -1) {
        throw Exception("Error creating TCP socket!");
    }

    this->efd = epoll_create1(0);
    if (this->efd == -1) {
        throw Exception("Error creating epoll instance!");
    }

    this->makeNonBlocking(this->sfd);

    listen(this->sfd, this->backlog);
    this->loop_thread = std::thread(&TCPServer::loop, this);
}

TCPServer::~TCPServer() {
    this->stop();
}

void TCPServer::stop() {
    this->active = false;
    this->loop_thread.join();
}

void TCPServer::processEvent(int i) {
    // These are exceptinal events
    if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
        DEBUG("Connection error: %s, %u",
            this->clients[events[i].data.fd]->toString().c_str(), events[i].events);
        this->closeRemote(events[i].data.fd);
        return;
    // A normal socket close event
    } else if (events[i].events & EPOLLRDHUP) {
        DEBUG("Connection closed: %s", this->clients[events[i].data.fd]->toString().c_str());
        this->closeRemote(events[i].data.fd);
        return;
    // A socket create event
    } else if (this->sfd == events[i].data.fd) {
        while (1) {
            struct sockaddr cli_addr;
            socklen_t cli_addr_len = sizeof(cli_addr);
            int newsockfd = accept(sfd, (struct sockaddr *) &cli_addr, &cli_addr_len);
            if (newsockfd == -1) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    DEBUG("Processed all connections!");
                    break;
                } else {
                    ERROR("Error occured on accept!");
                    break;
                }
            }

            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
            getnameinfo(&cli_addr, cli_addr_len,
                       hbuf, sizeof hbuf,
                       sbuf, sizeof sbuf,
                       NI_NUMERICHOST | NI_NUMERICSERV);


            this->makeNonBlocking(newsockfd);

            TCPRemoteClient *c = new TCPRemoteClient(newsockfd, hbuf, atoi(sbuf));
            DEBUG("Connection created: %s", c->toString().c_str());

            if (this->onConnectionOpen) {
                if (!this->onConnectionOpen(c)) {
                    delete(c);
                    return;
                }
            }

            this->clients[newsockfd] = c;
            event.data.fd = newsockfd;
            event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
            if (epoll_ctl(this->efd, EPOLL_CTL_ADD, newsockfd, &event) == -1) {
                ERROR("Error adding connectino to epoll instance!");
            }
        }
    } else {
        int done = 0;

        while (1) {
            ssize_t count;
            char buf[512];

            count = read(events[i].data.fd, buf, sizeof buf);
            if (count == -1) {
                if (errno != EAGAIN) {
                    DEBUG("Error on read!");
                    done = 1;
                }
                break;
            } else if (count == 0) {
                done = 1;
                break;
            }

            DEBUG("Recievied %i bytes...", count);
            TCPRemoteClient *c = this->clients[events[i].data.fd];
            c->buffer.insert(c->buffer.end(), buf, buf + count);
            if (this->onConnectionData) {
                this->onConnectionData(c);
            }

            if (done) {
                DEBUG("Done reading from connection...");
                this->closeRemote(events[i].data.fd);
            }
        }
    }
}

void TCPServer::loop() {
    // Allocate events array
    events = (epoll_event *) calloc (MAXEVENTS, sizeof event);

    // First, add the server-socket file descriptor to epoll instance
    event.data.fd = this->sfd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(this->efd, EPOLL_CTL_ADD, this->sfd, &event) == -1) {
        throw Exception("Failed to epoll server socket fd!");
    }

    // Jesus is jealous of this loop
    while (this->active) {
        int n = epoll_wait(efd, events, MAXEVENTS, -1);

        for (int i = 0; i < n; i ++) {
            this->processEvent(i);
        }
    }

    free(events);
    close(this->sfd);
}

bool TCPServer::makeNonBlocking(int sfd) {
    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        ERROR("Failed to get flags on socket!");
        return false;
    }

    flags |= O_NONBLOCK;

    if (fcntl(sfd, F_SETFL, flags) == -1) {
        ERROR("Failed to set flags on socket!");
        return false;
    }

    return true;
}

void TCPServer::closeRemote(int rid) {
    TCPRemoteClient *c = this->clients[rid];
    if (this->onConnectionClose) {
        this->onConnectionClose(c);
    }

    this->clients.erase(this->clients.find(rid));
    delete(c);
}

void RemoteClient::tryParse() {
    if (!this->tcp->buffer.size()) {
        return;
    }

    // GC: This is cleaned up in the server parsing loop
    cubednet::Packet *packet = new cubednet::Packet;
    if (!packet->ParseFromArray(&this->tcp->buffer[0], this->tcp->buffer.size())) {
        WARN("Failed to parse data, assuming we need more...");
        return;
    }

    DEBUG("PACKET: %i, DATA-SIZE: %i", packet->pid(), packet->data().size());
    this->packet_buffer.push(packet);
}

TCPClient::TCPClient(std::string host, ushort port) {
    this->remote_host = host;
    this->remote_port = port;
}

bool TCPClient::conn() {
    struct sockaddr_in servaddr;
    this->fd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(this->remote_host.c_str());
    servaddr.sin_port = htons(this->remote_port);

    int i = connect(this->fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    DEBUG("Connect result %i", i);

    if (i == -1) {
        ERROR("Failed to connect...");
        return false;
    }

    if (this->onConnectionOpen) {
        this->onConnectionOpen();
    }

    this->read_loop_thread = std::thread(&TCPClient::read_loop, this);

    return true;
}

void TCPClient::read_loop() {
    char buffer[1024];
    int n;

    while (this->active) {
        n = recvfrom(this->fd, buffer, sizeof buffer, 0, NULL, NULL);
        if (n == 0) {
            WARN("Remote server disconnected...");
            this->active = false;
            break;
        }

        this->buffer.insert(this->buffer.end(), buffer, buffer + n);
        if (this->onConnectionData != nullptr) {
            this->onConnectionData();
        }
        DEBUG("Recieved %i bytes...", n);
    }

    if (this->onConnectionClose != nullptr) {
        this->onConnectionClose();
    }
}

void TCPClient::send_packet(int id, google::protobuf::Message *data) {
    std::string buffa, buffb;
    cubednet::Packet pk;
    pk.set_pid(id);

    data->SerializeToString(&buffa);

    if (this->our_kp && this->serv_kp) {
        std::string nonce = this->our_kp->newNonce();
        std::string encd = this->our_kp->encrypt(buffa, nonce, (* this->serv_kp));
        pk.set_data(encd);
        pk.set_nonce(nonce);
    } else {
        pk.set_data(buffa);
    }

    pk.SerializeToString(&buffb);
    write(this->fd, buffb.c_str(), buffb.size());
}

TCPClient::~TCPClient() {
    if (this->serv_kp != nullptr) delete(serv_kp);
    this->active = false;
    alarm(1);
    this->read_loop_thread.join();
    close(this->fd);
}

void RemoteClient::disconnect(int r, const std::string d) {
    DEBUG("Disconnecting remote client for reason %i, decs: %s",
        r, d.c_str());
    cubednet::PacketDisconnect dc;
    dc.set_reason(r);
    dc.set_desc(d);
    this->tcp->send_packet(PACKET_DC, &dc);
    this->tcp->disconnect();
}

void RemoteClient::terminate() {
    DEBUG("Terminating connection to remote client");
    this->tcp->disconnect();
}

void TCPRemoteClient::send_packet(int id, google::protobuf::Message *data) {
    std::string buffa, buffb;
    cubednet::Packet pk;
    pk.set_pid(id);

    data->SerializeToString(&buffa);

    if (this->remote->state > STATE_NEW) {
        std::string nonce = this->remote->serv_kp->newNonce();
        std::string encd = this->remote->serv_kp->encrypt(buffa, nonce,
            (* this->remote->our_kp));
        pk.set_data(encd);
        pk.set_nonce(nonce);
    } else {
        pk.set_data(buffa);
    }

    pk.SerializeToString(&buffb);
    write(this->fd, buffb.c_str(), buffb.size());
}
