#pragma once

#include "global.h"
#include "world.h"
#include "net.h"
#include "util.h"
#include "renderer.h"
#include "crypto.h"
#include "loginserver.h"

class ClientConfig {
    public:
        std::string login_server;
        std::string uid;

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

        // Networking
        std::string remote_host;
        short remote_port;

        ClientWorld *world;
        ClientConfig config;
        LoginServer *loginserver;
        TCPClient *tcpcli;

        Window *main_window;

        std::string session;

        Client() {
            tick_rate = 64;
            fps_rate = 120;
        };

        ~Client() {
            this->shutdown();
            delete(this->world);
            delete(this->loginserver);
            delete(this->tcpcli);
            delete(this->main_window);
        }

        bool onConnectionData();
        bool onConnectionClose();
        bool onConnectionOpen();

        bool setup();
        void run();
        void main_loop();
        void shutdown();

        void connect(std::string);
};
