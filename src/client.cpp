#pragma once

#include "client.h"

void Client::connect(std::string addr) {
    // Load client configuration
    this->config.load();

    // Generate the connection string and attempt the connection
    Net::ConnString cs = Net::ConnString(addr);

    DEBUG("Client attempting connection to %s", cs.toString().c_str());
    this->tcp = new Net::TCPConnection(this->loop, cs);
    this->tcp->addEventCallback(std::bind(&Client::onTCPEvent, this, std::placeholders::_1));
    this->tcp->connect();
}

void Client::run() {
    this->active = true;
    this->connect("127.0.0.1:6060");
    this->loop->loop();
    // TODO: start thread
    // this->main_loop();
}

void Client::main_loop() {
    Ticker t = Ticker(32);

    SDL_Event e;

    while (this->active) {
        if (!t.next()) {
            WARN("RUNNING SLOW!");
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                DEBUG("Got SDL_QUIT...");
                this->active = false;
            }
        }
    }
}

bool Client::setup() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        ERROR("Failed to setup SDL!");
        return false;
    }

    // Request OpenGL 3.2
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    // Double buffering??
    // SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);

    this->main_window = new Window("Cubed", 460, 460);
}

void Client::shutdown() {
    DEBUG("Joining %i thread-pool threads", THREAD_POOL.size());
    for (auto &t : THREAD_POOL) {
        t->join();
    }

    SDL_Quit();
}

void Client::onTCPEvent(Net::TCPEvent& event) {
    switch (event.type) {
        case Net::TCP_CONNECT:
            INFO("Client TCP socket is connected. Sending handshake...");
            this->sendBeginHandshake();
            break;
        case Net::TCP_DISCONNECT:
            INFO("Client TCP socket is disconnected");
            break;
        case Net::TCP_MESSAGE:
            muduo::string msg(event.buffer->retrieveAllAsString());
            this->parseData(msg);
            break;
    }
}

void Client::sendPacket(ProtoNet::PacketType type, google::protobuf::Message *message) {
    DEBUG("Sending packet %i to server", type);
    std::string headerBuff, baseBuff;
    ProtoNet::Packet packet;

    // First, serialize the inner packet
    message->SerializeToString(&baseBuff);

    packet.set_type(type);
    packet.set_data(baseBuff);
    packet.SerializeToString(&headerBuff);

    this->tcp->send(headerBuff);
}

void Client::sendBeginHandshake() {
    // TODO: eventuall we will handle regaining state
    assert(this->state == CLIENT_INACTIVE);

    ProtoNet::PacketBeginHandshake handshake;

    handshake.set_username(this->config.username);
    handshake.set_fingerprint("TEST");
    handshake.set_version(CUBED_VERSION);

    this->sendPacket(ProtoNet::BeginHandshake, &handshake);
    this->state = CLIENT_SENT_HANDSHAKE;
}

void Client::sendCompleteHandshake(std::string password = "") {
    assert(this->state == CLIENT_SENT_HANDSHAKE);

    ProtoNet::PacketCompleteHandshake complete;
    complete.set_password(password);
    this->sendPacket(ProtoNet::CompleteHandshake, &complete);
    this->state = CLIENT_SENT_COMPLETION;
}

void Client::sendRequestRegion(BoundingBox b) {
    ProtoNet::PacketRequestRegion pkt;
    pkt.set_world_id(this->world->id);
    pkt.set_allocated_area(b.to_proto());
    this->sendPacket(ProtoNet::RequestRegion, &pkt);
}

void Client::parseData(muduo::string& data) {
    ProtoNet::Packet packet;

    if (!packet.ParseFromArray(&data[0], data.size())) {
        WARN("Failed to parse incoming packet: `%s`", data.c_str());
        return;
    }

    std::string innerBuff = packet.data();

    switch (packet.type()) {
        case ProtoNet::Invalid: {
            ERROR("Invalid packet recieved!");
            break;
        }
        case ProtoNet::Error: {
            ProtoNet::PacketError inner;
            inner.ParseFromArray(&innerBuff[0], innerBuff.size());
            this->onPacketError(inner);
            break;
        }
        case ProtoNet::AcceptHandshake: {
            ProtoNet::PacketAcceptHandshake inner;
            inner.ParseFromArray(&innerBuff[0], innerBuff.size());
            this->onPacketAcceptHandshake(inner);
            break;
        }
        case ProtoNet::Begin: {
            ProtoNet::PacketBegin inner;
            inner.ParseFromArray(&innerBuff[0], innerBuff.size());
            this->onPacketBegin(inner);
            break;
        }
        case ProtoNet::Region: {
            ProtoNet::PacketRegion inner;
            inner.ParseFromArray(&innerBuff[0], innerBuff.size());
            this->onPacketRegion(inner);
            break;
        }
        default: {
            WARN("Recieved unknown packet %i (data-size: %i)", packet.type(), packet.data().size());
        }
    }
}

void Client::onPacketError(ProtoNet::PacketError err) {
    if (err.type() == ProtoNet::Generic) {
        ERROR("Recieved generic error from the server: %s", err.msg().c_str());
        return;
    }

    ERROR("Recieved error %i from server: %s", err.type(), err.msg().c_str());
}

void Client::onPacketAcceptHandshake(ProtoNet::PacketAcceptHandshake pkt) {
    std::string password;

    if (this->state != CLIENT_SENT_HANDSHAKE) {
        throw Exception("Got unexpected PacketAcceptHandshake!");
    }

    // Save the client ID
    this->id = pkt.id();

    if (pkt.password()) {
        // TODO: grab from UI? throw error?
        password = "test";
    }

    // TODO: sign with our keypair

    // Send the end of our three-way handshake
    this->sendCompleteHandshake(password);
}

void Client::onPacketBegin(ProtoNet::PacketBegin pkt) {
    // TODO: set position and world
    this->world = new Terra::ClientWorld(pkt.world());
    this->sendRequestRegion(BoundingBox(
        Point(-12, 0, -12),
        Point(12, 12, 12)));
}

void Client::onPacketRegion(ProtoNet::PacketRegion pkt) {
    DEBUG("got region!");
    // TODO: load this region into the block cache
}

void ClientConfig::load() {
    Document d;
    loadJSONFile("client.json", &d);

    this->username = d["username"].GetString();
    this->login_server = d["login_server"].GetString();
    this->uid = d["uid"].GetString();
    this->auth = d["auth"].GetBool();
}
