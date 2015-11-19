#include "terra/terra.h"

namespace Terra {

// Load this block from the database
Block::Block(World *world, BlockType* type, Point p) {
    this->world = world;
    this->type = type;
    this->pos = p;
}

World::World(std::string path) {
    this->path = path;
    this->gen = new FlatGenerator("seed", this);
}

void World::open() {
    bool new_world = false;

    // First, lets check if the world path exists
    struct stat buffer;
    if (stat(this->path.c_str(), &buffer) != 0) {
        INFO("World does not exist yet, creating for the first time...");
        if (mkdir(this->path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
            ERROR("Failed to create world path at %s", this->path.c_str());
            throw Exception("Failed to create new world.");
        }

        if (mkdir(ioutil::join(path, "chunks").c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
            throw Exception("Failed to create chunks directory for new world.");
        }

        new_world = true;

        fp = fopen(ioutil::join(path, "world.json").c_str(), "w");
        if (!fp) {
            ERROR("Failed to open world file at %s", path.c_str());
            throw Exception("Failed to open worlld file");
        }

        // TODO: get the name from the path
        this->name = "world";
        this->version = CURRENT_WORLD_VERSION;
        this->seed = "testseed";
        this->dump();
    } else {
        INFO("Loading world file at %s", path.c_str());
        fp = fopen(ioutil::join(path, "world.json").c_str(), "r");

        if (!fp) {
            ERROR("Failed to open world file at %s", path.c_str());
            throw Exception("Failed to open worlld file");
        }
    }

    // We take out an exclusive lock on the world file to ensure we avoid corruption
    if (flock(fileno(fp), LOCK_EX | LOCK_NB) != 0) {
        if (errno == EWOULDBLOCK) {
            throw Exception("Failed to obtain world lock, someone else owns it");
        } else if (errno == EINVAL) {
            WARN("FAILED TO GET WORLD LOCK:");
            WARN("This probablly means you have an unsupported filesystem.");
            WARN("We'll continue, but operating this way is VERY dangerous and may cause corruption.");
        } else {
            ERROR("Failed to obtain world lock, reason: %i", errno);
            throw Exception("Failed to obtain world lock, see log for details");
        }
    }

    if (!new_world) {
        // Read and parse the JSON world description
        char readBuffer[65536];
        FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        Document d;

        try {
            d.ParseStream(is);
            this->name = d["name"].GetString();
            this->version = (unsigned short) d["version"].GetInt();
            this->seed = d["seed"].GetString();
            INFO("Loaded world %s version %i", name.c_str(), version);
        } catch (std::string e) {
            ERROR("Failed to load world info file: %s", e.c_str());
            throw Exception("Failed to load world info file");
        }

        if (version < MIN_SUPPORTED_WORLD_VERSION) {
            throw Exception("World file version is too old, needs conversion");
        }
    }

    // Now load the database
    std::string dbpath = ioutil::join(path, "blocks.db");
    rocksdb::Options options;
    options.create_if_missing = true;
    auto status = rocksdb::DB::Open(options, dbpath, &this->db);

    if (!status.ok()) {
        throw Exception("Failed to load blocks database for world");
    }

    // Make sure we the type ID incrementer is set
    std::string blocktypeid;
    status = this->db->Get(rocksdb::ReadOptions(), "meta:blocktypeid", &blocktypeid);

    if (!status.ok()) {
        this->db->Put(rocksdb::WriteOptions(), "meta:blocktypeid", "0");
    }
}

void World::dump() {
    Document d;
    d.SetObject();
    Document::AllocatorType& allocator = d.GetAllocator();

    Value name;
    name.SetString(this->name.c_str(), this->name.size(), allocator);
    d.AddMember("name", name, allocator);

    Value seed;
    seed.SetString(this->seed.c_str(), this->seed.size(), allocator);
    d.AddMember("seed", seed, allocator);

    d.AddMember("version", Value((int) this->version).Move(), allocator);

    char writeBuffer[1024];
    FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

    Writer<FileWriteStream> writer(os);
    d.Accept(writer);
}


void World::close() {
    if (!fp) { return; }

    INFO("Closing world file %s", name.c_str());
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    DEBUG("Closing world db...");
    delete(db);
}

Block* World::create_block(Point p, std::string type) {
    if (!this->holder->haveType(type)) {
        WARN("someone tried to create a block with an invalid type of %s", type.c_str());
        return nullptr;
    }

    return World::create_block(p, this->holder->getBlockType(type));
}

Block* World::create_block(Point p, BlockType* type) {
    return new Block(this, type, p);
}

Block* World::get_block(Point p) {
    std::string type;
    auto status = this->db->Get(rocksdb::ReadOptions(), Block::key(p), &type);

    // If the block isn't in the database, lets assume its air
    if (!status.ok() && status.code() == 1) {
        return this->create_block(p, "base:air");
    }

    if (!this->holder->haveType(type)) {
        return nullptr;
    }

    return new Block(this, this->holder->getBlockType(type), p);
}

void World::save_block(Block *blk) {
    if (blk == nullptr) {
        throw Exception("Cannot save_block on null block");
    }

    auto status = this->db->Put(rocksdb::WriteOptions(), Block::key(blk->pos), blk->type->name);
    assert(status.ok());
}

std::string Block::key(Point p) {
    char buffer[1024];
    sprintf(buffer, "block:%0.4f:%0.4f:%0.4f", p.x, p.y, p.z);
    return std::string(buffer);
}

void World::generateInitialWorld() {
    // Generate 16x16
    BoundingBox b = BoundingBox(Point(-256, 0, -256), Point(256, 256, 256));
    this->gen->generate_area(b);

    rocksdb::Iterator* it = this->db->NewIterator(rocksdb::ReadOptions());
    int c = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        c++;
    }
    delete it;
    INFO("total: %i", c);
}

ClientWorld::ClientWorld(ProtoNet::IWorld world) {
    this->id = world.id();
    this->name = world.name();
}

}
