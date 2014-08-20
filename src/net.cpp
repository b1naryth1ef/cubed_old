#include "net.h"

bool UDPService::open(std::string addr, int port) {
    DEBUG("Opening new udp service %s:%i", addr.c_str(), port);
    struct sockaddr_in serviceaddr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        ERROR("Error opening socket: %i", fd);
        throw Exception("Error opening socket!");
    }

    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    bzero((char *) &serviceaddr, sizeof(serviceaddr));
    serviceaddr.sin_family = AF_INET;
    serviceaddr.sin_addr.s_addr = inet_addr(addr.c_str());
    serviceaddr.sin_port = htons((unsigned short) port);

    int bd = bind(fd, (struct sockaddr *) &serviceaddr, sizeof(serviceaddr));
    if (bd < 0) {
        ERROR("Error binding socket: %i", bd);
        throw Exception("Error binding to address!");
    }

    this->active = true;
    this->read_loop_thread = std::thread(&UDPService::read_loop, this);
}

void UDPService::read_loop() {
    DEBUG("Started UDPService read_loop thread");

    struct sockaddr_in clientaddr;
    socklen_t len;
    int n;
    bool ok;

    char unsigned buffer[4096]; // = (char *) malloc(4096);

    while (this->active) {
        len = sizeof(clientaddr);
        n = recvfrom(fd, buffer, 1024, 0, (struct sockaddr *)&clientaddr, &len);
        if (!n) {
            continue;
        }

        buffer[n] = 0;
        ok = cubed::VerifyPacketBuffer(flatbuffers::Verifier(buffer, n));

        if (!ok) {
            WARN("Something weird happen in deseriliztion of packet...");
            continue;
        }

        auto packet = cubed::GetPacket(buffer);
        DEBUG("Recieved: %s", packet->tag());
    }

    DEBUG("Exiting from UDPService.read_loop");
}

bool UDPService::close(int reason) {
    this->active = false;
    shutdown(fd, SHUT_RDWR);
    this->read_loop_thread.join();
}