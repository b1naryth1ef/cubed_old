#include "server.h"

Server::Server() : dex(db) {
    INFO("Cubed Server v%i.%i.%i (%i, git: %s)",
        CUBED_RELEASE_A, CUBED_RELEASE_B, CUBED_RELEASE_C, CUBED_VERSION,
        CUBED_GIT_HASH);

    // Load the server cvars
    this->loadCvars();
    this->config.load();

    // Open up the server-db (player storage/etc)
    this->db = new DB("server.db");
    this->db->init_sqlite();
    if (this->db->is_new) {
        this->loadDatabase();
    }

    this->sv_tickrate->set(this->config.tickrate);
    this->sv_name->set(this->config.name);

    // Load the base types
    this->loadBaseTypes();

    // Load the worlds we need
    for (auto world_name : this->config.worlds) {
        this->loadWorld(world_name);
        // Terra::World *w = new Terra::World(world_name);
        // w->open();
        // this->addWorld(w);
    }

    // Load the mod-index
    for (auto mod_name : this->config.mods) {
        this->dex.loadFromPath(mod_name);
    }

    // Create a new networking event loop
    this->loop = new muduo::net::EventLoop();

    // Create a new TCP server, which will be the main entry point for shit
    this->tcps = new Net::TCPServer(this->loop, Net::ConnString(this->config.host_name, this->config.host_port));
    this->tcps->addEventCallback(std::bind(&Server::onTCPEvent, this, std::placeholders::_1));
    this->tcps->start();
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
    this->loop->loop();
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
    for (auto w : this->worlds) {
        //w.second->tick();
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

void Server::loadWorld(std::string name) {
    auto world = new Terra::World(name);

    // Set the blocktypeholder
    world->holder = this;

    // Make sure a world with this name is not already loaded
    for (auto w : this->worlds) {
        if (w.second->name == name) {
            throw Exception("Failed to loadWorld, world with name already exists");
        }
    }

    // Actually open and load the world
    world->open();

    // Set the world id
    this->world_id_inc++;
    world->id = this->world_id_inc;

    auto blk = world->get_block(Point(0, 0, 0));

    // TODO: This sucks that we have to have a block at 0,0,0 AND that we use the static string :/
    if (blk == nullptr || blk->type->name == "base:air") {
        INFO("We haven't loaded this world before, generating land...");
        world->generateInitialWorld();
    } else {
        INFO("World has been loaded before...");
    }

    this->worlds[world->id] = world;
    DEBUG("Server added world '%s' (%i).", world->name.c_str(), world->id);

    // TODO: Worlds should have their own update threads, we need to spawn
    //  that here.
}

void Server::loadDatabase() {
    int err = SQLITE_OK;
    char* errStr = NULL;

    err = sqlite3_exec(db->db, "CREATE TABLE players ("
            "id INTEGER PRIMARY KEY ASC,"
            "fingerprint TEXT,"
            "username TEXT,"
            "x INTEGER,"
            "y INTEGER,"
            "z INTEGER,"
            "world INTEGER,"
            "group_id INTEGER);", 0, 0, &errStr);

    if (err != SQLITE_OK) {
        ERROR("sqlite3_exec error: %s", errStr);
        throw Exception("Failed to load the server sqlite database");
    }
}

void ServerConfig::load() {
    Document d;
    loadJSONFile("server.json", &d);

    this->name = d["name"].GetString();
    this->host_name = d["host"]["name"].GetString();
    this->host_port = d["host"]["port"].GetInt();

    if (d.HasMember("tickrate")) {
        this->tickrate = d["tickrate"].GetInt();
    }

    if (d.HasMember("password")) {
        this->password = d["password"].GetString();
    }

    const Value& wrlds = d["worlds"];
    for (Value::ConstValueIterator itr = wrlds.Begin(); itr != wrlds.End(); ++itr) {
        this->worlds.push_back(itr->GetString());
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

void Server::onTCPEvent(Net::TCPEvent &event) {
    RemoteClient *rc = nullptr;

    switch (event.type) {
        case Net::TCP_CONNECT:
            this->pending.insert(new RemoteClient(event.client, this));
            break;
        case Net::TCP_DISCONNECT:
            for (auto client : this->clients) {
                if (client.second->client == event.client) {
                    rc = client.second;
                    break;
                }
            }

            // If we didn't find a remoteclient, something is fucky
            if (rc == nullptr) {
                WARN("Recieved TCP_DISCONNECT for untracked connection");
                return;
            }

            delete(rc);
            break;
        case Net::TCP_MESSAGE:
            break;
    }
}

void Server::addClient(RemoteClient *remote) {
    DBQuery* query = this->db->query("SELECT id, x, y, z, world FROM players WHERE fingerprint=?");
    query->bindText(remote->fingerprint);
    query->execute();

    if (!query->empty()) {
        INFO("Found an existing player for fingerprint %s (%i).", remote->fingerprint.c_str(), query->getInt(0));
        remote->id = query->getInt(0);
        remote->pos.x = query->getInt(1);
        remote->pos.y = query->getInt(2);
        remote->pos.z = query->getInt(3);

        remote->world = nullptr;
        std::string worldName = query->getText(4);
        for (auto w : this->worlds) {
            if (w.second->name == worldName) {
                remote->world = w.second;
            }
        }

        if (remote->world == nullptr) {
            WARN("Player is in an unloaded world, placing them in default spawn");
            throw Exception("TODO!");
        }
    } else {
        INFO("Inserting new player for fingerprint %s.", remote->fingerprint.c_str());
        DBQuery* insert = this->db->query(
            "INSERT INTO players (fingerprint, username, group_id, world, x, y, z) VALUES (?, ?, ?, ?, ?, ?, ?)");
        insert->bindText(remote->fingerprint);
        insert->bindText(remote->username);
        insert->bindInt(0);

        // TODO: use spawn point
        remote->pos = Point(0, 2, 0);
        remote->world = this->worlds[1];

        insert->bindText(remote->world->name);
        insert->bindInt(remote->pos.x);
        insert->bindInt(remote->pos.y);
        insert->bindInt(remote->pos.z);
        insert->execute();
        remote->id = insert->getLastInsertID();
        insert->end();
    }

    query->end();

    // Now add the user to our clients mapping
    this->pending.erase(remote);
    this->clients_mutex.lock();
    this->clients[remote->id] = remote;
    this->clients_mutex.unlock();
}

RemoteClient::RemoteClient(Net::TCPServerClient *client, Server *server) {
    this->client = client;
    this->server = server;
    this->client->addEventCallback(std::bind(&RemoteClient::onTCPEvent, this, std::placeholders::_1));
}

RemoteClient::~RemoteClient() {
    DEBUG("Destroying RemoteClient");
    if (id) {
        INFO("[%i] deregistering self in deconstructor", id);
        server->clients_mutex.lock();
        server->clients.erase(id);
        server->clients_mutex.unlock();
    } else {
        server->pending.erase(this);
    }
}

void RemoteClient::onTCPEvent(Net::TCPEvent& event) {
    if (event.type == Net::TCP_MESSAGE) {
        muduo::string msg(event.buffer->retrieveAllAsString());
        this->parseData(msg);
    }
}

void RemoteClient::parseData(muduo::string& data) {
    ProtoNet::Packet packet;

    // Attempt to parse the packet
    if (!packet.ParseFromArray(&data[0], data.size())) {
        WARN("Failed to parse data");
        return;
    }

    std::string innerBuff = packet.data();

    switch (packet.type()) {
        case ProtoNet::Invalid: {
            ERROR("Invalid packet recieved!");
            break;
        }
        case ProtoNet::Error: {
            ProtoNet::PacketError inner;
            inner.ParseFromArray(&innerBuff[0], innerBuff.size());
            this->onPacketError(inner);
            break;
        }
        case ProtoNet::BeginHandshake: {
            ProtoNet::PacketBeginHandshake inner;
            inner.ParseFromArray(&innerBuff[0], innerBuff.size());
            this->onPacketBeginHandshake(inner);
            break;
        }
        case ProtoNet::CompleteHandshake: {
            ProtoNet::PacketCompleteHandshake inner;
            inner.ParseFromArray(&innerBuff[0], innerBuff.size());
            this->onPacketCompleteHandshake(inner);
            break;
        }
        case ProtoNet::RequestRegion: {
            ProtoNet::PacketRequestRegion inner;
            inner.ParseFromArray(&innerBuff[0], innerBuff.size());
            this->onPacketRequestRegion(inner);
            break;
        }
        default: {
            DEBUG("PACKET: %i, DATA-SIZE: %i", packet.type(), packet.data().size());
        }
    }

}

void RemoteClient::sendPacket(ProtoNet::PacketType type, google::protobuf::Message *message) {
    DEBUG("Sending packet %i to remote", type);
    std::string headerBuff, baseBuff;
    ProtoNet::Packet packet;

    // First, serialize the inner packet
    message->SerializeToString(&baseBuff);

    packet.set_type(type);
    packet.set_data(baseBuff);
    packet.SerializeToString(&headerBuff);

    if (!this->client->alive()) {
        throw Exception("Cannot send packet to a non-connected client!");
    }

    this->client->send(headerBuff);
}

void RemoteClient::sendError(ProtoNet::ErrorType type, std::string msg = "") {
    ProtoNet::PacketError err;
    err.set_type(type);
    err.set_msg(msg);
    this->sendPacket(ProtoNet::Error, &err);
}

void RemoteClient::sendAcceptHandshake() {
    ProtoNet::PacketAcceptHandshake pkt;
    pkt.set_id(this->id);
    pkt.set_password((this->server->config.password != ""));

    this->challenge = random_string(32);
    pkt.set_challenge(this->challenge);
    this->sendPacket(ProtoNet::AcceptHandshake, &pkt);
}

void RemoteClient::sendBegin() {
    ProtoNet::PacketBegin pkt;
    pkt.mutable_world()->set_id(this->world->id);
    pkt.mutable_world()->set_name(this->world->name);
    pkt.mutable_pos()->set_x(this->pos.x);
    pkt.mutable_pos()->set_y(this->pos.y);
    pkt.mutable_pos()->set_z(this->pos.z);
    this->sendPacket(ProtoNet::Begin, &pkt);
}

void RemoteClient::sendRegion(Terra::World *world, BoundingBox b) {
    ProtoNet::IRegion region;

    region.set_world(world->id);
    region.set_allocated_box(b.to_proto());

    // Iterate over all the blocks in the region and add them to the packet
    for (int x = b.min.x; x < b.max.x; x++) {
        for (int y = b.min.y; y < b.max.y; y++) {
            for (int z = b.min.z; z < b.max.z; z++) {
                Terra::Block* blk = this->world->get_block(Point(x, y, z));
                region.add_blocks(blk->type->id);
            }
        }
    }

    ProtoNet::PacketRegion pkt;
    pkt.set_allocated_region(&region);
    DEBUG("sending region");
    this->sendPacket(ProtoNet::Region, &pkt);
}

void RemoteClient::onPacketError(ProtoNet::PacketError err) {
    if (err.type() == ProtoNet::Generic) {
        ERROR("Recieved generic error from the client: %s", err.msg().c_str());
        return;
    }
}


void RemoteClient::onPacketBeginHandshake(ProtoNet::PacketBeginHandshake pkt) {
    if (this->state != REMOTE_STATE_INVALID) {
        throw Exception("Recieved unexpected PacketBeginhandshake from client");
    }

    if (pkt.version() != CUBED_VERSION) {
        char buff[256];
        sprintf(&buff[0], "Invalid cubed version, expecting %i", CUBED_VERSION);
        this->sendError(ProtoNet::InvalidVersion, std::string(buff));
        return;
    }

    std::string username = pkt.username();
    if (username.size() > 32 || username.size() < 5) {
        this->sendError(ProtoNet::InvalidUsername, "Invalid username length");
        return;
    }

    // TODO: check we don't already have a client by this username

    this->fingerprint = pkt.fingerprint();

    // Send accept handshake
    this->server->addClient(this);
    this->sendAcceptHandshake();
    this->state = REMOTE_STATE_HANDSHAKE;
}

void RemoteClient::onPacketCompleteHandshake(ProtoNet::PacketCompleteHandshake pkt) {
    if (this->state != REMOTE_STATE_HANDSHAKE) {
        throw Exception("Recieved unexpected PacketCompleteHandshake from client");
    }

    if (this->server->config.password != "" && pkt.password() != this->server->config.password) {
        this->sendError(ProtoNet::InvalidPassword, "Invalid password");
        return;
    }

    // TODO: verify solution

    this->state = REMOTE_STATE_CONNECTED;

    // TODO: send begin state
    this->sendBegin();
}

void RemoteClient::onPacketRequestRegion(ProtoNet::PacketRequestRegion pkt) {
    DEBUG("Client has requested region...");
    // TODO: validation
    this->sendRegion(this->server->worlds[pkt.world_id()], BoundingBox(pkt.area()));

}

void RemoteClient::disconnect(DisconnectReason reason, const std::string text = "") {

}


