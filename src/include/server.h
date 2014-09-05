#pragma once

#include "global.h"
#include "world.h"
#include "net.h"
#include "util.h"
#include "cvar.h"
#include "mod.h"
#include "db.h"
#include "crypto.h"

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
        ushort client_id_inc = 1;

        std::map<std::string, ServerWorld*> worlds;

        std::mutex clients_mutex;
        std::map<int, RemoteClient*> clients;

        std::thread main_thread;
        bool active;

        TCPServer *tcps;
        KeyPair keypair = KeyPair("keys");

        ModDex dex;
        CVarDict *cvars;
        ServerConfig config;
        DB *db;

        // Server cvar handles
        CVar *sv_cheats;
        CVar *sv_name;
        CVar *sv_tickrate;
        CVar *sv_version;

        void addWorld(ServerWorld *);

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

        ushort newClientID();

        bool onTCPConnectionClose(TCPRemoteClient *c);
        bool onTCPConnectionOpen(TCPRemoteClient *c);
        bool onTCPConnectionData(TCPRemoteClient *c);

        void handlePacket(cubednet::Packet *pk, RemoteClient *c);
};
