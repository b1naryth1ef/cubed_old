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

    DEBUG("Client has host `%s` and port `%d`", this->remote_host.c_str(), this->remote_port);
    this->tcpcli.conn(this->remote_host, this->remote_port);
}

void Client::run() {
    this->active = true;
    this->connect("127.0.0.1:6060");
    this->main_loop();
}

void Client::main_loop() {
    Ticker t = Ticker(32);

    while (this->active) {
        if (!t.next()) {
            WARN("RUNNING SLOW!");
        }

        // this->tick();
    }
}

bool Client::setup() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        ERROR("Failed to setup SDL!");
        return false;
    }

    this->main_window = new Window("Cubed", 460, 460);
}
