#include "server.h"

Server::Server() {
    INFO("Cubed Server v%i.%i.%i (%i, git: %s)",
        CUBED_RELEASE_A, CUBED_RELEASE_B, CUBED_RELEASE_C, CUBED_VERSION,
        CUBED_GIT_HASH);

    // Load the SQLite3 Module stuff TODO: move/rename?
    init_db_module();

    // Load the servers keypairs
    if (this->keypair.empty) {
        INFO("Generating new server keypair");
        this->keypair.generate();
        this->keypair.save("keys");
    }

    // Load the server cvars
    this->loadCvars();
    this->config.load();

    // Load and validate the login servers
    THREAD([this]() {
        INFO("Loading all login servers...");

        for (auto &ls : this->config.login_servers) {
            LoginServer *serv = new LoginServer(ls);
            if (serv->valid) {
                this->login_servers.push_back(serv);
            }
        }

        if (!this->login_servers.size()) {
            WARN("No login servers provided, authentication/sessions are disabled!");
        }
    });

    // Open up the server-db (player storage/etc)
    this->db = new DB("server.db");
    if (this->db->is_new) {
        this->loadDatabase();
    }

    this->sv_tickrate->set(this->config.tickrate);
    this->sv_name->set(this->config.name);

    // Load the base types
    this->loadBaseTypes();

    // Load the worlds we need
    for (auto world_name : this->config.worlds) {
        Terra::World *w = new Terra::World(world_name);
        w->open();
        this->addWorld(w);
    }


    // Load the mod-index
    this->dex.db = this->db;
    for (auto mod_name : this->config.mods) {
        this->dex.loadFromPath(mod_name);
    }

    // Create a new TCP server, which will be the main entry point for shit
    this->tcps = new TCPServer(this->config.host_name, this->config.host_port);
    this->tcps->onConnectionOpen = std::bind(&Server::onTCPConnectionOpen, this,
        std::placeholders::_1);
    this->tcps->onConnectionClose = std::bind(&Server::onTCPConnectionClose, this,
        std::placeholders::_1);
    this->tcps->onConnectionData = std::bind(&Server::onTCPConnectionData, this,
        std::placeholders::_1);
}

Server::~Server() {
    this->shutdown();
}

void Server::shutdown() {
    for (auto v : this->worlds) {
        v.second->close();
    }

    for (auto v : this->login_servers) {
        delete(v);
    }

    DEBUG("Joining %i thread-pool threads", THREAD_POOL.size());
    for (auto &t : THREAD_POOL) {
        t->join();
    }
}

void Server::serveForever() {
    this->active = true;
    this->main_thread = std::thread(&Server::mainLoop, this);
}

/*
    It's assumed that everything within this thread has parent access to
    anything in server. Any other threads should use locks if they need to
    access things, or queue things to this thread.
*/
void Server::mainLoop() {
    Ticker t = Ticker(this->sv_tickrate->getInt());

    while (this->active) {
        if (!t.next()) {
            WARN("RUNNING SLOW!");
        }

        this->tick();
    }
}

void Server::tick() {
    this->clients_mutex.lock();
    for (auto c : this->clients) {
        if (c.second->packet_buffer.size()) {
            DEBUG("Parsing one packet...");
            cubednet::Packet *p = c.second->packet_buffer.front();
            c.second->packet_buffer.pop();
            this->handlePacket(p, c.second);
            delete(p);
        }
    }
    this->clients_mutex.unlock();

    for (auto w : this->worlds) {
        // w.second->tick();
    }
}

void Server::loadCvars() {
    this->cvars = new CVarDict();

    this->cvars->bind(std::bind(&Server::onCVarChange, this,
        std::placeholders::_1,
        std::placeholders::_2));

    // TODO: implement this
    this->cvars->load("server.json");

    sv_cheats = cvars->create("sv_cheats", "Allow changing of cheat protected variables");
    sv_name = cvars->create("sv_name", "A name for the server");

    sv_tickrate = cvars->create("sv_tickrate", "The servers tickrate");
    sv_tickrate->rmvFlag(FLAG_USER_WRITE)->rmvFlag(FLAG_MOD_WRITE);

    sv_version = cvars->create("sv_version", "The servers version");
    sv_version->set(CUBED_VERSION);
    sv_version->rmvFlag(FLAG_USER_WRITE)->rmvFlag(FLAG_MOD_WRITE);

}

void Server::addWorld(Terra::World *w) {
    if (this->worlds.count(w->name)) {
        ERROR("Cannot add world with name %s, another world with that"
            "name already exists!", w->name.c_str());
        throw Exception("Failed to add world to server, already exists!");
    }

    // Copy the types over
    for (auto type : this->types) {
        w->addBlockType(type.second);
    }

    auto blk = w->get_block(Point(0, 0, 0));

    // TODO: This sucks that we have to have a block at 0,0,0 AND that we use the static string :/
    if (blk == nullptr || blk->type->name == "base:air") {
        INFO("We haven't loaded this world before, generating land...");
        w->generateInitialWorld();
    } else {
        INFO("World has been loaded before...");
    }

    this->worlds[w->name] = w;

    // TODO: Worlds should have their own update threads, we need to spawn
    //  that here.
}

void Server::loadDatabase() {
    int err = SQLITE_OK;

    // err = sqlite3_exec(db->db, "CREATE TABLE mods ("
    //     "id INTEGER PRIMARY KEY ASC,"
    //     "name TEXT,"
    //     "version INTEGER"
    // ");", 0, 0, 0);

    assert(err == SQLITE_OK);
}

void ServerConfig::load() {
    Document d;
    loadJSONFile("server.json", &d);

    this->name = d["name"].GetString();
    this->host_name = d["host"]["name"].GetString();
    this->host_port = d["host"]["port"].GetInt();
    this->tickrate = d["tickrate"].GetInt();

    const Value& wrlds = d["worlds"];
    for (Value::ConstValueIterator itr = wrlds.Begin(); itr != wrlds.End(); ++itr) {
        this->worlds.push_back(itr->GetString());
    }

    const Value& logins = d["logins"];
    for (Value::ConstValueIterator itr = logins.Begin(); itr != logins.End(); ++itr) {
        this->login_servers.push_back(itr->GetString());
    }
    const Value& mods = d["mods"];
    for (Value::ConstValueIterator itr = mods.Begin(); itr != mods.End(); ++itr) {
        this->mods.push_back(itr->GetString());
    }
}

bool Server::onCVarChange(CVar *cv, Container *new_value) {
    DEBUG("onCVarChange!");
    return false;
};

bool Server::onTCPConnectionOpen(TCPRemoteClient *c) {
    DEBUG("Adding TCPClient...");
    RemoteClient *rc = new RemoteClient();
    rc->state = STATE_NEW;
    rc->tcp = c;
    rc->id = c->id = this->newClientID();
    c->remote = rc;

    this->clients_mutex.lock();
    this->clients[rc->id] = rc;
    this->clients_mutex.unlock();

    return true;
}

bool Server::onTCPConnectionClose(TCPRemoteClient *c) {
    DEBUG("Removing TCPClient...");
    this->clients_mutex.lock();
    delete(this->clients[c->id]);
    this->clients.erase(c->id);
    this->clients_mutex.unlock();

    return true;
}

bool Server::onTCPConnectionData(TCPRemoteClient *c) {
    DEBUG("Running tryparse on client %i", c->id);
    this->clients[c->id]->tryParse();
    return true;
}

ushort Server::newClientID() {
    while (this->clients[client_id_inc]) {
        client_id_inc++;
    }

    return client_id_inc;
}

void Server::handlePacket(cubednet::Packet *pk, RemoteClient *c) {
    std::string data;

    // If we're connected, the data MUST be encrypted!
    if (c->state == STATE_CONNECTED) {
        data = this->keypair.decrypt(
            pk->data(),
            pk->nonce(),
            (* c->our_kp));
    } else {
        data = pk->data();
    }

    switch (pk->pid()) {
        case PACKET_HANDSHAKE: {
            cubednet::PacketHandshake pkh;
            assert(pkh.ParseFromString(data));
            this->handlePacketHandshake(pkh, c);
            break;
        }
        case PACKET_STATUS_REQUEST: {
            cubednet::PacketStatusRequest pkh;
            assert(pkh.ParseFromString(data));
            this->handlePacketStatusRequest(pkh, c);
            break;
        }
    }

}

void Server::handlePacketHandshake(cubednet::PacketHandshake pk, RemoteClient *c) {
    DEBUG("Client has version %i, we have %i!", pk.version(), CUBED_VERSION);
    if (pk.version() != CUBED_VERSION) {
        c->disconnect(1, "Invalid Cubed Version!");
        return;
    }

    if (c->state != STATE_NEW) {
        c->disconnect(1, "Generic Protocol Error.");
        return;
    }

    if (pk.session() != 0) {
        c->disconnect(1, "Login servers are not supported yet!");
        return;
    }

    c->state = STATE_HANDSHAKE;
    c->our_kp = new KeyPair();
    c->our_kp->loadFromString("", pk.pubkey());
    c->serv_kp = &this->keypair;

    // Send a init packet, this is now encrypted
    cubednet::PacketInit pkinit;
    pkinit.set_id(c->id);
    c->tcp->send_packet(PACKET_INIT, &pkinit);
}

void Server::handlePacketStatusRequest(cubednet::PacketStatusRequest pk, RemoteClient *c) {
    // Make sure the junk data is the right size. This is used to prevent
    //  spam/dos attacks against the server by ensuring the request packet
    //  is always larger than the servers response.
    if (pk.data().size() < STATUS_JUNK_DATA_SIZE) {
        c->terminate();
    }

    cubednet::PacketStatusResponse res;
    res.set_name(this->sv_name->getString());
    res.set_motd(this->sv_motd->getString());
    res.set_players(this->clients.size());
    res.set_version(this->sv_version->getInt());

    // Set the public key
    res.set_pubkey(this->keypair.getPublicKey());

    char buffer[1024];
    sprintf(buffer, "%Ld|%Ld|%i", pk.a1(), pk.a2(), 0);

    cubednet::SignedMessage *sm(res.mutable_data());
    this->keypair.sign(buffer).toPacket(sm);

    c->tcp->send_packet(PACKET_STATUS_RESPONSE, &res);
}

void Server::addBlockType(Terra::BlockType *type) {
    if (this->types.count(type->name)) {
        WARN("addBlockType replacing type %s", type->name.c_str());
    }

    this->types[type->name] = type;

    // For each world,
    for (auto world : this->worlds) {
        world.second->addBlockType(type);
    }
}

void Server::rmvBlockType(std::string type) {
    this->types.erase(type);
}

Terra::BlockType* Server::getBlockType(std::string type) {
    return this->types[type];
}

void Server::loadBaseTypes() {
    this->addBlockType(new Terra::AirType());
    this->addBlockType(new Terra::BedRockType());
}
