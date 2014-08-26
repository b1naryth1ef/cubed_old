#include "server.h"

Server::Server(std::string world_name, std::string name, int tickrate) {
    DEBUG("Cubed v%i.%i.%i", CUBED_RELEASE_A, CUBED_RELEASE_B, CUBED_RELEASE_C);

    // Load the SQLite3 Module stuff
    init_db_module();

    // Load the server cvars
    this->load_cvars();

    // Load the world (this is for testing)
    this->world = new World(world_name);
    this->world->load();

    this->sv_tickrate->setInt(tickrate);

    this->udp_s = new UDPService();
    this->udp_s->open("0.0.0.0", 6060);

    this->dex.loadFromPath("vanilla");
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
    Ticker t = Ticker(this->sv_tickrate->getInt());

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

void Server::load_cvars() {
    this->cvars = new CVarDict();
    //this->cvars->bind(this->onCVarChange);

    // TODO: implement this
    this->cvars->load("server.json");

    // We make sure a bunch of stuff is loaded
    this->cvars->get("sv_cheats");
    this->cvars->get("sv_hostname");
    this->cvars->get("sv_name");
    this->sv_tickrate = this->cvars->get("sv_tickrate");

    CVar *sv_version = this->cvars->get("sv_version");
    sv_version->setInt(CUBED_VERSION);
    sv_version->rmvFlag(FLAG_USER_WRITE)->rmvFlag(FLAG_MOD_WRITE);
}

bool Server::onCVarChange(CVar *cv, Container *from_value, Container *to_value) {};
