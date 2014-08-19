#pragma once

#include "global.h"
#include "world.h"

class Server {
    public:
        // TODO: allow multiple worlds
        World *world;

        // Server info
        std::string s_name;
        int s_version;

        int tickrate;

        Server(std::string world_name, std::string name);

        void loop();
};