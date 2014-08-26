#pragma once

#include "global.h"
#include "world.h"
#include "net.h"
#include "util.h"
<<<<<<< HEAD
#include "cvar.h"
=======
#include "mod.h"
>>>>>>> 2a186e061cf464a36b4c1a3c53b2ca29cd74f4b1

class Server {
    public:
        // TODO: allow multiple worlds
        World *world;
        std::thread main_thread, net_thread;
        bool active;

        UDPService *udp_s;
        TCPService *tcp_s;

        ModDex dex;

        CVarDict *cvars;

        CVar *sv_tickrate;

        void load_cvars();

        Server(std::string world_name, std::string name, int tickrate);
        ~Server();

        void serve_forever();
        void tick();

        void main_loop();
        void net_loop();
<<<<<<< HEAD

        // Cvar bindings
        bool onCVarChange(CVar *cv, Container *from_value, Container *to_value);
};
=======
};
>>>>>>> 2a186e061cf464a36b4c1a3c53b2ca29cd74f4b1
