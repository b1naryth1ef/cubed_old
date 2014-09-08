#pragma once

#include "global.h"
#include "world.h"
#include "net.h"
#include "util.h"
#include "renderer.h"

class Client {
    public:
        bool active;
        int tick_rate;
        int fps_rate;

        // Networking
        std::string remote_host;
        short remote_port;

        ClientWorld *world;
        TCPClient tcpcli;

        Window *main_window;

        Client() {
            tick_rate = 64;
            fps_rate = 120;
        };

        ~Client() {
            this->shutdown();
        }

        bool setup();
        void run();
        void main_loop();
        void shutdown();

        void connect(std::string);
};