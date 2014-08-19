#include "net.h"

bool UDPService::open(std::string addr, int port) {
    LOG.L("Opening new udp service %s:%i", addr.c_str(), port);
    struct sockaddr_in serviceaddr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        throw Exception("Error opening socket!");
    }

    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    bzero((char *) &serviceaddr, sizeof(serviceaddr));
    serviceaddr.sin_family = AF_INET;
    serviceaddr.sin_addr.s_addr = inet_addr(addr.c_str());
    serviceaddr.sin_port = htons((unsigned short) port);

    if (bind(fd, (struct sockaddr *) &serviceaddr, sizeof(serviceaddr)) < 0) {
        throw Exception("Error binding to address!");
    }
}

bool UDPService::close(int reason) {
    shutdown(fd, SHUT_RDWR);
}