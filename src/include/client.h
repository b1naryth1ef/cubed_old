#pragma once

#include "net/util.h"
#include "net/tcp.h"

#include "util/util.h"
#include "util/crypto.h"

#include "global.h"
#include "world.h"
#include "renderer.h"
#include "loginserver.h"

#include <muduo/net/EventLoop.h>

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

        // Networking event loop
        muduo::net::EventLoop *loop;

        // The login-server UID for this user
        std::string login_uid;

        KeyPair keypair = KeyPair("ckeys");
        KeyPair *serv_kp;

        // ClientWorld *world;
        ClientConfig config;
        LoginServer *loginserver;
        // TCPClient *tcpcli;
        Net::TCPConnection *tcp;

        Window *main_window;

        int session;

        Client() {
            this->loop = new muduo::net::EventLoop;
            tick_rate = 64;
            fps_rate = 120;
        };

        ~Client() {
            this->shutdown();
            // delete(this->world);
            delete(this->loginserver);
            // delete(this->tcpcli);
            delete(this->main_window);

            delete(this->loop);
            delete(this->tcp);
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
