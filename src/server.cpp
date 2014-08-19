#include "server.h"

Server::Server(std::string world_name, std::string name) {
    WorldFile wf(world_name);
    wf.open();

    this->world = new World(&wf);
    this->s_name = name;
    this->s_version = CUBED_VERSION;
}