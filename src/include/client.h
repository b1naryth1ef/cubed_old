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

        // Represents all blocks in-memory, air blocks must exist in this
        BlockCacheT blocks;

        Client() {
            tick_rate = 64;
            fps_rate = 120;
        };

        void run();
        void main_loop();

        void connect(std::string);
};