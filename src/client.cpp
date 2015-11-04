#pragma once

#include "client.h"

void Client::connect(std::string addr) {
    std::vector<std::string> arr = splitString(addr, ':');

    if (arr.size() == 0) {
        throw Exception("Invalid Host String!");
    }

    if (arr.size() >= 1) {
        this->remote_host = arr[0];
    }

    if (arr.size() == 2) {
        std::string::size_type sz;
        this->remote_port = (short) std::stoi(arr[1], &sz);
    }

    // Setup data directory
    ioutil::setupDataDirectory();

    // Load client configuration
    this->config.load();

    // Load loginserver and validate it
    this->loginserver = new LoginServer(this->config.login_server);

    // if (this->keypair.empty) {
    //     INFO("Generating new c keypair");
    //     this->keypair.generate();
    //     this->keypair.save("keys");
    // }

    DEBUG("Client has host `%s` and port `%d`", this->remote_host.c_str(), this->remote_port);
    this->tcpcli = new TCPClient(this->remote_host, this->remote_port);
    this->tcpcli->onConnectionData = std::bind(&Client::onConnectionData, this);
    this->tcpcli->onConnectionClose = std::bind(&Client::onConnectionClose, this);
    this->tcpcli->onConnectionOpen = std::bind(&Client::onConnectionOpen, this);
    this->tcpcli->conn();
}

void Client::run() {
    this->active = true;
    this->connect("127.0.0.1:6060");
    this->main_loop();
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
    delete(this->main_window);
}

bool Client::onConnectionData() {
    if (!this->tcpcli->buffer.size()) {
        return true;
    }

    // GC: This is cleaned up in the server parsing loop
    cubednet::Packet *packet = new cubednet::Packet;
    if (!packet->ParseFromArray(&this->tcpcli->buffer[0], this->tcpcli->buffer.size())) {
        WARN("Failed to parse data, assuming we need more...");
        return true;
    }

    this->handlePacket(packet);

    DEBUG("PACKET: %i, DATA-SIZE: %i", packet->pid(), packet->data().size());
}

bool Client::onConnectionClose() {
    DEBUG("Connection closed!");
}

bool Client::onConnectionOpen() {
    cubednet::PacketStatusRequest pksr;

    unsigned char junk[STATUS_JUNK_DATA_SIZE];
    randombytes_buf(junk, sizeof junk);
    pksr.set_data((const char *) junk);

    // TODO: track state
    pksr.set_a1(randombytes_uniform(MAX_UINT32));
    pksr.set_a2(randombytes_uniform(MAX_UINT32));


    // cubednet::PacketHandshake pk;
    // pk.set_version(CUBED_VERSION);

    // if (!this->config.auth) {
    //     INFO("Login Servers are disabled, attempting blank auth.");

    //     if (this->keypair.empty) {
    //         this->keypair.generate();
    //     }
    //     pk.set_session(0);
    //     pk.set_loginserver("");
    //     pk.set_pubkey(this->keypair.getPublicKey());
    // } else {
    //     INFO("Attempting to authenticate with login server...");

    //     if (this->keypair.empty) {
    //         DEBUG("Creating a new account...");
    //         this->login_uid = this->loginserver->createAccount(&this->keypair);
    //         this->keypair.save("ckeys");
    //     }

    //     DEBUG("Creating login session...");
    //     this->login_uid = this->config.uid;
    //     this->session = this->loginserver->login(this->login_uid, this->keypair);
    //     pk.set_session(this->session);
    //     pk.set_loginserver(this->loginserver->url);
    // }

    // DEBUG("Saving client keypair...");
    // this->keypair.save(ioutil::join(ioutil::getDataDirectory(), "keys"));

    // DEBUG("Sending handshake packet...");
    // this->tcpcli->send_packet(PACKET_HANDSHAKE, &pk);
}

void Client::handlePacket(cubednet::Packet *pk) {
    std::string data;

    if (pk->nonce() != "") {
        data = this->keypair.decrypt(
            pk->data(),
            pk->nonce(),
            (*this->serv_kp));
    } else {
        data = pk->data();
    }

    switch (pk->pid()) {
        case PACKET_INIT: {
            cubednet::PacketInit pkh;
            assert(pkh.ParseFromString(data));
            this->handlePacketInit(&pkh);
        }
        case PACKET_STATUS_RESPONSE: {
            cubednet::PacketStatusResponse pkh;
            assert(pkh.ParseFromString(data));
            this->handlePacketStatusResponse(&pkh);
        }
    }
}

void Client::handlePacketInit(cubednet::PacketInit *pk) {

}

void Client::handlePacketStatusResponse(cubednet::PacketStatusResponse *pk) {

}


void ClientConfig::load() {
    Document d;
    loadJSONFile("client.json", &d);

    this->login_server = d["login_server"].GetString();
    this->uid = d["uid"].GetString();
    this->auth = d["auth"].GetBool();
}
