#pragma once

#include "util/util.h"
#include "util/crypto.h"

#include "global.h"
#include "world.h"
#include "net.h"
#include "renderer.h"
#include "loginserver.h"

class ClientConfig {
    public:
        std::string login_server;
        std::string uid;
        bool auth;

        void load();
};

class Client {
    public:
        bool active;
        int tick_rate;
        int fps_rate;

        // The login-server UID for this user
        std::string login_uid;

        KeyPair keypair = KeyPair("ckeys");
        KeyPair *serv_kp;

        // Networking
        std::string remote_host;
        short remote_port;

        // ClientWorld *world;
        ClientConfig config;
        LoginServer *loginserver;
        TCPClient *tcpcli;

        Window *main_window;

        int session;

        Client() {
            tick_rate = 64;
            fps_rate = 120;
        };

        ~Client() {
            this->shutdown();
            // delete(this->world);
            delete(this->loginserver);
            delete(this->tcpcli);
            delete(this->main_window);
        }

        bool onConnectionData();
        bool onConnectionClose();
        bool onConnectionOpen();

        void handlePacket(cubednet::Packet *pk);
        void handlePacketInit(cubednet::PacketInit *pk);
        void handlePacketStatusResponse(cubednet::PacketStatusResponse *pk);

        bool setup();
        void run();
        void main_loop();
        void shutdown();

        void connect(std::string);
};
