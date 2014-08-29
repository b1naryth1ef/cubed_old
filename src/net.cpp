#include "net.h"

TCPServer::TCPServer(std::string host, short port) {
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr);

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        throw Exception("Error binding port");
    }

    this->makeNonBlocking(sockfd);

    listen(sockfd, this->backlog);
    this->loop_thread = std::thread(&TCPServer::loop, this);
}

TCPServer::~TCPServer() {
    this->stop();
}

void TCPServer::stop() {
    this->active = false;
    this->loop_thread.join();
}

// TODO: fucking refactor
void TCPServer::loop() {
    struct epoll_event event;
    struct epoll_event *events;

    this->efd = epoll_create1(0);

    if (this->efd == -1) {
        throw Exception("Failed to epoll_create1");
    }

    event.data.fd = this->sockfd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(this->efd, EPOLL_CTL_ADD, this->sockfd, &event) == -1) {
        throw Exception("Failed to epoll server socket fd!");
    }

    events = (epoll_event *) calloc (MAXEVENTS, sizeof event);

    while (this->active) {
        int n = epoll_wait(efd, events, MAXEVENTS, -1);

        for (int i = 0; i < n; i ++) {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                DEBUG("Error occured on %i, %u", i, events[i].events);
                close(events[i].data.fd);
                continue;
            } else if (events[i].events & EPOLLRDHUP) {
                DEBUG("Socket was closed");
                close(events[i].data.fd);
                continue;
            } else if (this->sockfd == events[i].data.fd) {
                while (1) {
                    struct sockaddr cli_addr;
                    socklen_t cli_addr_len = sizeof(cli_addr);
                    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_addr_len);
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
                    int s = getnameinfo(&cli_addr, cli_addr_len,
                               hbuf, sizeof hbuf,
                               sbuf, sizeof sbuf,
                               NI_NUMERICHOST | NI_NUMERICSERV);

                    DEBUG("Accepted connection host=%s, port=%s", hbuf, sbuf);
                    this->makeNonBlocking(newsockfd);

                    event.data.fd = newsockfd;
                    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                    if (epoll_ctl(this->efd, EPOLL_CTL_ADD, newsockfd, &event) == -1) {
                        ERROR("error epolling new conn");
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

                    DEBUG("Input: %i, %s |", count, buf);
                    if (done) {
                        DEBUG("Done reading from connection...");
                        close(events[i].data.fd);
                    }
                }
            }
        }
    }

    free(events);
    close(this->sockfd);
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