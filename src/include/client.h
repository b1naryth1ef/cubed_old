#pragma once

#include "net/util.h"
#include "net/tcp.h"

#include "util/util.h"

#include "global.h"
#include "renderer.h"

#include "packet.pb.h"

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

        // KeyPair keypair = KeyPair("ckeys");
        // KeyPair *serv_kp;

        // ClientWorld *world;
        ClientConfig config;
        Net::TCPConnection *tcp;

        Window *main_window = nullptr;

        int session;

        Client() {
            this->loop = new muduo::net::EventLoop;
            tick_rate = 64;
            fps_rate = 120;
        };

        ~Client() {
            this->shutdown();
            // delete(this->world);
            // delete(this->tcpcli);

            if (this->main_window != nullptr) {
                delete(this->main_window);
            }

            delete(this->loop);
            delete(this->tcp);
        }

        void onTCPEvent(Net::TCPEvent&);

        void sendPacket(ProtoNet::PacketType, google::protobuf::Message*);

        // void handlePacket(ProtoNet::Packet *pk);
        // void handlePacketInit(ProtoNet::PacketInit *pk);
        // void handlePacketStatusResponse(ProtoNet::PacketStatusResponse *pk);

        void sendBeginHandshake();

        bool setup();
        void run();
        void main_loop();
        void shutdown();

        void connect(std::string);
};
