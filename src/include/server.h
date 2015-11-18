#pragma once

#include "global.h"

#include <muduo/net/EventLoop.h>

// #include "net.h"
#include "cvar.h"
#include "mod.h"
#include "db.h"

#include "packet.pb.h"

#include "net/tcp.h"

#include "terra/terra.h"
#include "terra/blocktype.h"

#include "util/util.h"

enum RemoteClientState {
    REMOTE_STATE_INVALID,
    REMOTE_STATE_HANDSHAKE,
    REMOTE_STATE_CONNECTED
};

enum DisconnectReason {
    DISCONNECT_GENERIC,
};

class Server;

// Represents a remote client playing on the server
class RemoteClient {
    private:
        // This is used to verify the clients keypair
        std::string challenge;

    public:
        RemoteClient(Net::TCPServerClient*, Server*);
        ~RemoteClient();

        // The world and position of this player
        Point pos;
        Terra::World *world;

        // The local client ID (per session)
        uint64_t id;

        // The client username. Chosen by the client
        std::string username;

        // The client keypair fingerprint
        std::string fingerprint;

        RemoteClientState state = REMOTE_STATE_INVALID;
        Net::TCPServerClient *client;
        Server *server;

        void sendPacket(ProtoNet::PacketType, google::protobuf::Message*);

        // Packet sending helpers
        void sendError(ProtoNet::ErrorType, std::string);
        void sendAcceptHandshake();
        void sendBegin();

        // Packet handling helpers
        void onPacketError(ProtoNet::PacketError);
        void onPacketBeginHandshake(ProtoNet::PacketBeginHandshake);
        void onPacketCompleteHandshake(ProtoNet::PacketCompleteHandshake);

        void onTCPEvent(Net::TCPEvent&);
        void parseData(muduo::string&);
        void disconnect(DisconnectReason, const std::string);
};

class ServerConfig {
    public:
        std::string name;
        std::string host_name;
        short host_port;

        std::string password = "";

        std::vector<std::string> worlds;
        uint8_t tickrate = 64;

        std::vector<std::string> login_servers;
        std::vector<std::string> mods;

        void load();
};

class Server {
    private:
        uint64_t client_id_inc = 0;

    public:
        // Set of clients pending completion of their handshake
        std::set<RemoteClient*> pending;

        // Loaded worlds
        std::map<std::string, Terra::World*> worlds;

        // Holds all registered block types
        Terra::BlockTypeCache types;

        // A mapping of connected clients
        std::mutex clients_mutex;
        std::map<uint64_t, RemoteClient*> clients;

        // The main thread
        std::thread main_thread;

        // Whether the server is active
        bool active;

        // Networking event loop
        muduo::net::EventLoop *loop;

        // The TCP server
        Net::TCPServer *tcps;

        // The servers keypair
        // KeyPair keypair = KeyPair("keys");

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
        void addWorld(Terra::World *);

        // Loads cvars, should be called early (startup)
        void loadCvars();

        // Loads the server-database
        void loadDatabase();

        // Loads the base types
        void loadBaseTypes();

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
        uint64_t newClientID();

        // Manage block types
        void addBlockType(Terra::BlockType*);
        void rmvBlockType(std::string);
        Terra::BlockType* getBlockType(std::string);

        void onTCPEvent(Net::TCPEvent&);

        void addClient(RemoteClient*);
};
