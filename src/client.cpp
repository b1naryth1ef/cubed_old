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
    DEBUG("Got data!");
}

bool Client::onConnectionClose() {
    DEBUG("Connection closed!");
}

bool Client::onConnectionOpen() {
    DEBUG("Creating login session...");
    if (this->keypair.empty) {
        this->login_uid = this->loginserver->createAccount(&this->keypair);
        this->keypair.save("ckeys");
    }

    this->login_uid = this->config.uid;
    this->session = this->loginserver->login(this->login_uid, this->keypair);
    // cubednet::PacketHello pkh;

    // pkh.set_username("test");
    // pkh.set_hashcode("0");
    // pkh.set_version(CUBED_VERSION);
    // this->send_packet(PACKET_HELLO, &pkh);
}

void ClientConfig::load() {
    Document d;
    loadJSONFile("client.json", &d);

    this->login_server = d["login_server"].GetString();
    this->uid = d["uid"].GetString();
}
