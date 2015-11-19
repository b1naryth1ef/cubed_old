#pragma once

#include "net/util.h"
#include "net/tcp.h"

#include "util/util.h"
#include "util/geo.h"

#include "terra/terra.h"

#include "global.h"
#include "renderer.h"

#include "packet.pb.h"

#include <muduo/net/EventLoop.h>

enum ClientState {
    CLIENT_INACTIVE,
    CLIENT_SENT_HANDSHAKE,
    CLIENT_SENT_COMPLETION,
    CLIENT_ACTIVE
};

class ClientConfig {
    public:
        std::string username;

        std::string login_server;
        std::string uid;
        bool auth;

        void load();
};

class Client {
    public:
        // Represents the current state of the client<->server connection
        ClientState state = CLIENT_INACTIVE;

        // Our client ID (provided and managed by the server)
        uint64_t id;

        // Is the rendering (core) loop active?
        bool active;

        // Networking event loop
        muduo::net::EventLoop *loop;

        // Config for the client
        ClientConfig config;

        // The core TCP connection
        Net::TCPConnection *tcp;

        // The current world
        Terra::ClientWorld *world;

        // Holds all registered block types
        Terra::BlockTypeCache types;

        Window *main_window = nullptr;

        int session;

        Client() {
            this->loop = new muduo::net::EventLoop;
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

        // Eventing and parsing
        void onTCPEvent(Net::TCPEvent&);
        void parseData(muduo::string&);

        // Used to send packets to the remote
        void sendPacket(ProtoNet::PacketType, google::protobuf::Message*);

        // Packet handlers
        void onPacketError(ProtoNet::PacketError);
        void onPacketAcceptHandshake(ProtoNet::PacketAcceptHandshake);
        void onPacketBegin(ProtoNet::PacketBegin);
        void onPacketRegion(ProtoNet::PacketRegion);

        // Utility methods for sending data over the wire
        void sendBeginHandshake();
        void sendCompleteHandshake(std::string);
        void sendRequestRegion(BoundingBox);

        // General stateful settings
        bool setup();
        void run();
        void main_loop();
        void shutdown();

        // Connect to a server specified by a hoststring
        void connect(std::string);

        // Manage block types

};
