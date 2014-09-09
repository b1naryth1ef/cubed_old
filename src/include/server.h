#pragma once

#include "global.h"
#include "world.h"
#include "net.h"
#include "util.h"
#include "cvar.h"
#include "mod.h"
#include "db.h"
#include "crypto.h"
#include "http.h"

#define STATUS_JUNK_DATA_SIZE 256

class ServerConfig {
    public:
        std::string name;
        std::string host_name;
        short host_port;

        std::vector<std::string> worlds;
        short tickrate;

        std::vector<std::string> login_servers;

        void load();
};

class Server {
    public:
        // This represents the global client ID counter
        ushort client_id_inc = 1;

        // This holds all the worlds loaded by the server
        std::map<std::string, ServerWorld*> worlds;

        // A mapping of connected clients
        std::mutex clients_mutex;
        std::map<ushort, RemoteClient*> clients;

        // The main thread
        std::thread main_thread;

        // Whether the server is active
        bool active;

        // The TCP server
        TCPServer *tcps;

        // The servers keypair
        KeyPair keypair = KeyPair("keys");

        // The mod index
        ModDex dex;

        // The servers cvar dictionary
        CVarDict *cvars;

        // The servers config
        ServerConfig config;

        // The servers database file
        DB *db;

        // Server cvar handles
        CVar *sv_cheats;
        CVar *sv_name;
        CVar *sv_tickrate;
        CVar *sv_version;
        CVar *sv_motd;

        Server();
        ~Server();

        // Adds a world to the server
        void addWorld(ServerWorld *);

        // Loads cvars, should be called early (startup)
        void loadCvars();

        // Loads the server-database
        void loadDatabase();

        // Shuts the server down
        void shutdown();

        // Runs the server (this is blocking)
        void serveForever();

        // Fires a server tick, should be run by the main loop
        void tick();

        // The main server loop
        void mainLoop();

        // Fired when CVarDict (this.cvars) has a change
        bool onCVarChange(CVar *, Container *);

        // Generates a new client-id
        ushort newClientID();

        bool verifyLoginServer(std::string&);

        // Hooks for the TCP-server
        bool onTCPConnectionClose(TCPRemoteClient *c);
        bool onTCPConnectionOpen(TCPRemoteClient *c);
        bool onTCPConnectionData(TCPRemoteClient *c);

        // Hooks for packets
        void handlePacket(cubednet::Packet *pk, RemoteClient *c);
        void handlePacketHello(cubednet::PacketHello pk, RemoteClient *c);
        void handlePacketStatusRequest(cubednet::PacketStatusRequest pk, RemoteClient *c);
};
