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
}

void Client::run() {

}

void Client::main_loop() {
    
}