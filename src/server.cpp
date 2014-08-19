#include "server.h"

Server::Server(std::string world_name, std::string name, int tickrate) {
    LOG.L("Cubed v%i.%i.%i", CUBED_RELEASE_A, CUBED_RELEASE_B, CUBED_RELEASE_C);

    // Load the SQLite3 Module stuff
    init_db_module();

    // Load the world stuff
    init_world_module();

    this->world = new World(world_name);
    this->s_name = name;
    this->s_version = CUBED_VERSION;
    this->tickrate = tickrate;
}

Server::~Server() {
    this->world->close();
}

void Server::serve_forever() {
    this->active = true;
    this->main_thread = std::thread(&Server::main_loop, this);
}

void Server::main_loop() {
    int tick_diff, time_per_tick = (1.0 / this->tickrate) * 1000;

    /*
        This is the main loop which processes ticks. The logic for this is
        fairly simple, namely we try and keep up with num(tickrate) per second,
        if we have spare time we just spend it sleeping. Generally this should
        allow us to never skip ticks and remain efficient. Ticks that take longer
        will immediatly fire the next tick allowing for calculations to even
        out over time.
    */
    while (this->active) {
        std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
        this->tick();
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds>(t1 - t0);
        tick_diff = ms.count();

        long int time_wait = (time_per_tick - tick_diff);
        LOG.L("Tick took %ims to process fully, spare %ims.", tick_diff, time_wait);

        if (tick_diff > time_per_tick) {
            LOG.L("[WARN] RUNNING SLOW! Attempting to catch up by skipping sleep...");
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(time_wait));
        }
    }
}

void Server::tick() {
    // std::this_thread::sleep_for(std::chrono::milliseconds(7));
}