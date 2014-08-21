#include "server.h"

Server::Server(std::string world_name, std::string name, int tickrate) {
    DEBUG("Cubed v%i.%i.%i", CUBED_RELEASE_A, CUBED_RELEASE_B, CUBED_RELEASE_C);

    // Load the SQLite3 Module stuff
    init_db_module();

    this->world = new World(world_name);
    this->world->type_index = new BlockTypeIndexT();
    load_default_block_types(this->world->type_index);
    this->world->load();

    this->s_name = name;
    this->s_version = CUBED_VERSION;
    this->tickrate = tickrate;

    this->udp_s = new UDPService();
    this->udp_s->open("0.0.0.0", 6060);
}

Server::~Server() {
    this->world->close();
    this->udp_s->close(-1);
}

void Server::serve_forever() {
    this->active = true;
    this->main_thread = std::thread(&Server::main_loop, this);
    this->net_thread = std::thread(&Server::net_loop, this);
}

/*
    It's assumed that everything within this thread has parent access to
    anything in server. Any other threads should use locks if they need to
    access things, or queue things to this thread.
*/
void Server::main_loop() {
    Ticker t = Ticker(this->tickrate);

    while (this->active) {
        if (!t.next()) {
            WARN("RUNNING SLOW!");
        }

        this->tick();
    }
}

void Server::tick() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Server::net_loop() {
    while (this->active) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}