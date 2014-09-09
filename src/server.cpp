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

    for (auto &ls : this->config.login_servers) {
        this->verifyLoginServer(ls);
    }

    this->db = new DB("server.db");
    if (this->db->is_new) {
        this->loadDatabase();
    }

    this->sv_tickrate->set(this->config.tickrate);
    this->sv_name->set(this->config.name);

    for (auto world_name : this->config.worlds) {
        ServerWorld *w = new ServerWorld(world_name);
        w->load();
        this->addWorld(w);
    }

    this->dex.db = this->db;
    this->dex.loadFromPath("vanilla");

    // Create a new TCP server, which will be the main entry point for shit
    this->tcps = new TCPServer(this->config.host_name, this->config.host_port);
    this->tcps->onConnectionOpen = std::bind(&Server::onTCPConnectionOpen, this,
        std::placeholders::_1);
    this->tcps->onConnectionClose = std::bind(&Server::onTCPConnectionClose, this,
        std::placeholders::_1);
    this->tcps->onConnectionData = std::bind(&Server::onTCPConnectionData, this,
        std::placeholders::_1);

    // Dict test;
    // test.setString("string", "test");
    // test.setInt("int", 1);
    // test.setDouble("double", 1.342342);
}

Server::~Server() {
    this->shutdown();
}

void Server::shutdown() {
    for (auto v : this->worlds) {
        v.second->close();
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
        w.second->tick();
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

void Server::addWorld(ServerWorld *w) {
    if (this->worlds.count(w->wf->name)) {
        ERROR("Cannot add world with name %s, another world with that"
            "name already exists!", w->wf->name.c_str());
        throw Exception("Failed to add world to server, already exists!");
    }

    this->worlds[w->wf->name] = w;

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

    const Value& logins = d["logins"];
    for (Value::ConstValueIterator itr = logins.Begin(); itr != logins.End(); ++itr) {
        this->login_servers.push_back(itr->GetString());
    }

    fclose(fp);
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
    switch (pk->pid()) {
        case PACKET_HELLO: {
            cubednet::PacketHello pkh;
            assert(pkh.ParseFromString(pk->data()));
            this->handlePacketHello(pkh, c);
            break;
        }
        case PACKET_STATUS_REQUEST: {
            cubednet::PacketStatusRequest pkh;
            assert(pkh.ParseFromString(pk->data()));
            this->handlePacketStatusRequest(pkh, c);
            break;
        }
    }

}

void Server::handlePacketHello(cubednet::PacketHello pk, RemoteClient *c) {
    DEBUG("Client has version %i, we have %i!", pk.version(), CUBED_VERSION);
    if (pk.version() != CUBED_VERSION) {
        c->disconnect(2, "Invalid Cubed Version!");
        return;
    }

    if (c->state != STATE_NEW) {
        c->disconnect(1, "Generic Protocol Error.");
        return;
    }

    c->state = STATE_HANDSHAKE;
    DEBUG("Would send handshake...");
    // TODO: send back handshaking packet
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
    res.set_data(this->keypair.sign(buffer));

    c->tcp->send_packet(PACKET_STATUS_RESPONSE, &res);
}

bool Server::verifyLoginServer(std::string &x) {
    HTTPClient cli;

    char buffer[x.size() + 10];
    sprintf(buffer, "%s/api/info", x.c_str());
    HTTPResponse r = cli.request(
        HTTPRequest(buffer)
    );

    if (r.code != 200) {
        ERROR("Failed to verify login server %s (%Li)", x.c_str(), r.code);
        return false;
    } else {
        DEBUG("Verified login server %s", x.c_str());
    }

    return true;
}
