#include "server.h"

Server::Server() {
    DEBUG("Cubed v%i.%i.%i", CUBED_RELEASE_A, CUBED_RELEASE_B, CUBED_RELEASE_C);

    // Load the SQLite3 Module stuff TODO: move/rename?
    init_db_module();

    // Load the server cvars
    this->loadCvars();
    this->config.load();

    this->db = new DB("server.db");
    if (this->db->is_new) {
        this->setupDatabase();
    }

    this->sv_tickrate->setInt(this->config.tickrate);
    this->sv_name->setString(this->config.name);

    for (auto world_name : this->config.worlds) {
        World *w = new World(world_name);
        w->load();
        this->addWorld(w);
    }


    this->dex.db = this->db;
    this->dex.loadFromPath("vanilla");
}

Server::~Server() {
    for (auto v : this->worlds) {
        v.second->close();
    }
}

void Server::serve_forever() {
    this->active = true;
    this->main_thread = std::thread(&Server::main_loop, this);
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
    for (auto w : this->worlds) {
        w.second->tick();
    }
}

void Server::loadCvars() {
    this->cvars = new CVarDict();
    // TODO: get this working
    //this->cvars->bind(this->onCVarChange);

    // TODO: implement this
    this->cvars->load("server.json");

    sv_cheats = cvars->create("sv_cheats", "Allow changing of cheat protected variables");
    sv_name = cvars->create("sv_name", "A name for the server");

    sv_tickrate = cvars->create("sv_tickrate", "The servers tickrate");
    sv_tickrate->rmvFlag(FLAG_USER_WRITE)->rmvFlag(FLAG_MOD_WRITE);

    sv_version = cvars->create("sv_version", "The servers version");
    sv_version->setInt(CUBED_VERSION);
    sv_version->rmvFlag(FLAG_USER_WRITE)->rmvFlag(FLAG_MOD_WRITE);

}

bool Server::onCVarChange(CVar *cv, Container *from_value, Container *to_value) {};

void Server::addWorld(World *w) {
    this->worlds[w->wf->name] = w;
}

void Server::setupDatabase() {
    int err;

    // err = sqlite3_exec(db->db, "CREATE TABLE mods ("
    //     "id INTEGER PRIMARY KEY ASC,"
    //     "name TEXT,"
    //     "version INTEGER"
    // ");", 0, 0, 0);

    assert(err == SQLITE_OK);
}

void ServerConfig::load() {
    FILE *fp = fopen("server.json", "r");

    if (!fp) {
        ERROR("Could not open server configuration file `server.json`!");
        throw Exception("Failed to load server configuration file!");
    }

    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;

    try {
        d.ParseStream(is);
    } catch (std::string e) {
        ERROR("Error occured while parsing server configuration: %s", e.c_str());
        throw Exception("Error occured while parsing server configuration!");
    }

    this->name = d["name"].GetString();
    this->host_name = d["host"]["name"].GetString();
    this->host_port = d["host"]["port"].GetInt();
    this->tickrate = d["tickrate"].GetInt();

    const Value& wrlds = d["worlds"];
    for (Value::ConstValueIterator itr = wrlds.Begin(); itr != wrlds.End(); ++itr) {
        this->worlds.push_back(itr->GetString());
    }


    fclose(fp);
}
