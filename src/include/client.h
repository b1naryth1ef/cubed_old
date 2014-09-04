#pragma once

#include "global.h"
#include "world.h"
#include "net.h"
#include "util.h"

class Client {
    public:
        bool active;
        int tick_rate;
        int fps_rate;

        // Networking
        std::string remote_host;
        short remote_port;

        // TODO: Eventaully we may support multiple worlds loaded at once on the client?
        ClientWorld *world;

        TCPClient tcpcli;

        Client() {
            tick_rate = 64;
            fps_rate = 120;
        };

        void run();
        void main_loop();

        void connect(std::string);
};