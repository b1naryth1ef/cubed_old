#pragma once

#include "global.h"
#include "world.h"
#include "net.h"
#include "util.h"
#include "cvar.h"
#include "mod.h"
#include "db.h"

class ServerConfig {
    public:
        std::string name;
        std::string host_name;
        short host_port;

        std::vector<std::string> worlds;
        short tickrate;

        void load();
};

class Server {
    public:
        std::map<std::string, World*> worlds;

        std::thread main_thread;
        bool active;

        TCPServer *tcps;

        ModDex dex;
        CVarDict *cvars;
        ServerConfig config;
        DB *db;

        // Server cvar handles
        CVar *sv_cheats;
        CVar *sv_name;
        CVar *sv_tickrate;
        CVar *sv_version;

        void addWorld(World *);

        void loadCvars();

        void setupDatabase();

        Server();
        ~Server();

        void shutdown();

        void serve_forever();
        void tick();

        void main_loop();
        void net_loop();

        bool onCVarChange(CVar *, Container *);
};
