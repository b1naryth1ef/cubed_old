#include "server.h"

Server::Server(std::string world_name, std::string name) {
    this->world = new World(world_name);
    this->s_name = name;
    this->s_version = CUBED_VERSION;
}

Server::~Server() {
    this->world->close();
}

void Server::serve_forever() {
    this->active = true;
    this->main_thread = std::thread(&Server::loop, this);
}

void Server::loop() {
    while (this->active) {
        this->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Server::tick() {
    LOG.L("TICK :D");
}