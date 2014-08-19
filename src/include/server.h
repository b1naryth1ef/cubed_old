#pragma once

#include "global.h"
#include "world.h"
#include "net.h"

class Server {
    public:
        // TODO: allow multiple worlds
        World *world;
        std::thread main_thread;
        bool active;

        // Server info
        std::string s_name;
        int s_version;

        int tickrate;

        Server(std::string world_name, std::string name, int tickrate);
        ~Server();

        void serve_forever();
        void tick();
        void main_loop();
        void net_loop();
};