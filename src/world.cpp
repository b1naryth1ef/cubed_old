#include "world.h"

// Loads the default null and air block types into a BlockTypeIndex
void load_default_block_types(World *w, BlockTypeIndexT *bti) {
    DEBUG("Loading default block types...");

    BlockType *null_bt = new BlockType("null", false);
    w->addBlockType(null_bt);

    BlockType *air_bt = new BlockType("air", false);
    air_bt->persists = false;
    w->addBlockType(air_bt);
}

// Creates a new block type from a blocktype SQLite row
BlockType::BlockType(sqlite3_stmt *res) {
    this->id = sqlite3_column_int(res, 0);
    this->type = (char *)sqlite3_column_text(res, 1);
    this->active = sqlite3_column_int(res, 2);

    cubedworld::BlockTypeExtra bte;

    char *data = (char *) sqlite3_column_blob(res, 3);

    if (sqlite3_column_bytes(res, 3) <= 0) {
        DEBUG("No extra data to read!");
        return;
    }

    bool ok = bte.ParseFromArray(data, sqlite3_column_bytes(res, 3));

    if (!ok) {
        ERROR("Failed to load extra BlockType data from sql!");
    }

    this->owner = bte.owner();
    this->persists = bte.persists();
}

// Creates a new WorldFile from a directory path
WorldFile::WorldFile(std::string dir) {
    directory = dir;
}

// Cleanly closes a WorldFile
WorldFile::~WorldFile() {
    close();
}

// Attempts to open a WorldFile
bool WorldFile::open() {
    DEBUG("Attempting to open worldfile...");
    fp = fopen(ioutil::join(this->directory, "world.json").c_str(), "r");


    // We could not properly open the file
    if (!fp) {
        ERROR("Failed to open world file!");
        throw Exception("Failed to open world file!");
    }

    // Create a lock for the file, which warns other instances of cubed to not write to
    //  this world file.
    int lock_result = flock(fileno(fp), LOCK_EX | LOCK_NB);
    if (lock_result != 0) {
        ERROR("Error getting lock: %i", lock_result);
        throw Exception("Error occured getting lock for world file!");
    }

    // Read the entire json document
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;

    try {
        d.ParseStream(is);
    } catch (std::string e) {
        ERROR("Error occured parsing json world file: %s", e.c_str());
        throw Exception("Error occured parsing json world file!");
    }

    // Parse the name and version
    this->name = d["name"].GetString();
    this->version = d["version"].GetInt();
    DEBUG("Name and version: %s, %i", name.c_str(), version);

    // Parse the origin point
    const Value& org = d["origin"];

    // Some minor validation of the array to avoid segfaults huehue
    if (!org.IsArray() or org.Size() != 3) {
        throw Exception("Invalid origin in world json...");
    }

    // Load the origin as a point
    this->origin = new Point(org);

    // Check the version is valid
    if (version < SUPPORTED_WORLD_FILE_VERSION) {
        throw Exception("World file version is older, would need to convert it!");
    } else if (version > SUPPORTED_WORLD_FILE_VERSION) {
        throw Exception("World file version is newer than the supported version,\
            this is a fatal error!");
    }

    std::string dbpath = ioutil::join(this->directory, "data.db");
    this->db = new DB(dbpath);

    // If this is a new db, create the tables
    if (this->db->is_new) {
        DEBUG("Creating database for first time...");
        this->setupDatabase();
    }

    // TODO: put this in a load?
    this->db->addCached("insert_block",
        "INSERT INTO blocks (type, x, y, z)"
        "VALUES (?, ?, ?, ?);");

    this->db->addCached("insert_blocktype",
        "INSERT INTO blocktypes (type, active, extra)"
        "VALUES (?, ?, ?);");

    this->db->addCached("find_block_pos",
        "SELECT * FROM blocks WHERE x=? AND y=? AND z=?");

    return true;
}

// Do a first-time setup on the WorldFile database
bool WorldFile::setupDatabase() {
    int err;

    err = sqlite3_exec(db->db, "CREATE TABLE blocktypes ("
        "id INTEGER PRIMARY KEY ASC,"
        "type TEXT,"
        "active INTEGER,"
        "extra BLOB"
    ");", 0, 0, 0);

    err = sqlite3_exec(db->db, "CREATE TABLE blocks ("
        "type INTEGER,"
        "x INTEGER,"
        "y INTEGER,"
        "z INTEGER,"
        "PRIMARY KEY (x, y, z)"
    ");", 0, 0, 0);

    // err = sqlite3_exec(db->db, "CREATE TABLE blockdata ();", 0, 0, 0);

    // err = sqlite3_exec(db->db, "CREATE INDEX blocks_full_coord ON blocks ("
    //     "x ASC, y ASC, z ASC"
    // ");", 0, 0, 0);

    if (err != SQLITE_OK) {
        throw Exception("Error creating first-time world database...");
    }

    return true;
}

// Safely close the WorldFile
bool WorldFile::close() {
    if (!fp) { return false; }

    // Unlock the file and close it
    DEBUG("Closing world file %s", name.c_str());
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    DEBUG("Closing db...");
    delete(db);

    DEBUG("Done closing worldfile!");

    return true;
}

// Create a new World from a WorldFile
World::World(WorldFile *wf) {
    this->wf = wf;
    this->db = wf->db;
}

// Load a new World and WorldFile from a path
World::World(std::string path) {
    this->wf = new WorldFile(path);
    this->wf->open();
    this->db = this->wf->db;
}

// Update the world for this tick
bool World::tick() {
    for (auto &e : this->entities) {
        if (e->keepWorldLoadedAround()) {
            // TOOD: generate the region of blocks we need to keep loaded
            //  on this entity
        }
    }

    this->blockQueueLock.lock();
    if (this->blockQueue.size()) {
        int incr = 0;
        DEBUG("Have %i queued blocks...", this->blockQueue.size());
        while (this->blockQueue.size()) {
            Block *b = this->blockQueue.front();
            this->blockQueue.pop();

            /*
                Very unlikely edge case, has to be loaded AFTER we put the
                block in the queue, but BEFORE the next world tick.
            */
            if (this->blocks.count(b->pos)) {
                WARN(
                    "A block in the blockQueue was already loaded, assuming "
                    "a previous entry is more accurate and skipping."
                );
                continue;
            }

            this->blocks[b->pos] = b;

            if (incr++ > 4096) {
                WARN("Too many blocks in queue, waiting tell next tick to load more...");
                break;
            }
        }
    }
    this->blockQueueLock.unlock();

    return true;
}

bool World::load() {
    int amount = 64;
    DEBUG("Loading world...");

    this->type_index = new BlockTypeIndexT();
    this->loadBlockTypeIndex();
    load_default_block_types(this, this->type_index);

    // Allocate a new Point vector, this is deleted in loadBlocksAsync
    PointV *initial = new PointV;
    for (int x = 0; x < amount; x++) {
        for (int y = 0; y < amount; y++) {
            for (int z = 0; z < amount; z++) {
                initial->push_back(Point(x, y, z));
            }
        }
    }

    // Fire off an async job to load in all the blocks
    this->loadBlocksAsync(initial, true);

    return true;
}

World::~World() {
    this->close();
}

bool World::close() {
    for (auto b : this->blocks) {
        if (b.second->dirty) {
            b.second->save();
        }
    }

    this->wf->close();
    return true;
}

/*
    This function takes a vector of points, and attempts to asynchrounously
    load all the blocks at the points in the array into the working blockset.
    If a block that is requested in this request is already loaded, it will
    NOT be reloaded and instead ignored (which may on a rare change cause
    a conflict, beware of this!). If cleanup is true, the function will
    cleanup the Point vector after it's finished.
*/
bool World::loadBlocksAsync(PointV *points, bool cleanup) {
    THREAD([this, points, cleanup](){
        Timer t = Timer();
        t.start();

        DEBUG("Starting asynchronous loading of %i points...", points->size());
        for (auto p : (*points)) {
            this->loadBlock(p, true);
        }
        DEBUG("Finished asynchronous loading of points in %ims!", t.end());

        if (cleanup) {
            delete(points);
        }
    });

    return true;
}

bool World::loadBlocks(PointV points) {
    for (auto i : points) {
        this->loadBlock(i);
    }

    return true;
}

/*
    Attempts to load a block at Point p. If the block exists in the cache,
    it will NOT be loaded and this will return false. If the block does not
    exist at all, a new air block will be created and added to BOTH the
    database and block cache. If safe is true, this will force the world
    to only pickup the new block on the next tick, which should be used
    for ANY async or threaded call to this function. In the case that safe
    is true, and the world loads the same block at Point P BEFORE the next
    tick is called, the block loaded within this call will be ignored.
*/
bool World::loadBlock(Point p, bool safe) {
    // If the block already exists, skip this
    if (blocks.count(p)) {
        return false;
    }

    Block *b;

    sqlite3_stmt *stmt = this->db->getCached("find_block_pos");
    sqlite3_bind_double(stmt, 1, p.x);
    sqlite3_bind_double(stmt, 2, p.y);
    sqlite3_bind_double(stmt, 3, p.z);

    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW) {
        b = new Block(this, stmt);
    } else if (s == SQLITE_DONE) {
        DEBUG("No block exists for %F, %F, %F! Assuming air block.", p.x, p.y, p.z);
        b = new Block(this, p, this->findBlockType("air"));
        b->save();
    } else {
        throw Exception("Failed to load block, query!");
    }

    if (safe) {
        this->blockQueueLock.lock();
        this->blockQueue.push(b);
        this->blockQueueLock.unlock();
    } else {
        blocks[p] = b;
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return true;
}

BlockType *World::findBlockType(std::string s) {
    BlockType *result = nullptr;
    for (auto &v : (*this->type_index)) {
        if (v.second->type == s) {

            // More than one result
            if (result != nullptr) {
                return nullptr;
            }

            result = v.second;
        }
    }

    if (result != nullptr) {
        return result;
    }

    return nullptr;
}

// TODO: move the blocktype saving into the blocktype class plz
bool World::addBlockType(BlockType *bt) {
    if (this->findBlockType(bt->type) != nullptr) {
        DEBUG("Cannot addBlockType on `%s`, already exists in index.", bt->type.c_str());
        return false;
    }

    // Add to type_index
    (*this->type_index)[bt->id] = bt;

    this->db->begin();

    sqlite3_stmt *stmt = this->db->getCached("insert_blocktype");

    sqlite3_bind_text(stmt, 1, bt->type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, 1);

    // Save extra data
    std::string data;
    cubedworld::BlockTypeExtra bte;
    bte.set_persists(bt->persists);
    bte.set_owner(bt->owner);
    bte.SerializeToString(&data);
    sqlite3_bind_blob(stmt, 3, data.c_str(), data.size(), NULL);

    int s = sqlite3_step(stmt);
    int id = sqlite3_last_insert_rowid(this->db->db);

    if (s != SQLITE_DONE) {
        ERROR("Failed to save blocktype!");
        throw Exception("Failed to save blocktype!");
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    this->db->end();

    bt->id = id;

    return true;
}

bool World::loadBlockTypeIndex() {
    DEBUG("Loading block types from index");
    sqlite3_stmt *stmt;
    const char *ztail;
    int s;

    int err = sqlite3_prepare_v2(db->db, "SELECT * FROM blocktypes ORDER BY id", 100, &stmt, &ztail);

    if (err != SQLITE_OK) {
        throw Exception("Failed to load block type index, initial query!");
    }

    while (1) {
        s = sqlite3_step(stmt);

        if (s == SQLITE_ROW) {
            BlockType *bt = new BlockType(stmt);

            if (this->findBlockType(bt->type) != nullptr) {
                WARN("Blocktype with name `%s` is already in index w/ id %i, skipping...",
                    bt->type.c_str(), bt->id);
                continue;
            }

            (*this->type_index)[bt->id] = bt;
            DEBUG("Loaded blocktype %i / %s from index", bt->id, bt->type.c_str());
        } else if (s == SQLITE_DONE) {
            break;
        } else {
            throw Exception("Failed to load block type index, query-parse!");
        }
    }

    sqlite3_finalize(stmt);
    return true;
}

/*
    Returns a block at Point p from the loaded block cache, if the block
    is not loaded or not in the cache, it will return NULL instead.
*/
Block *World::getBlock(Point p)  {
    if (blocks.count(p) != 1) {
        return NULL;
    }

    return blocks.find(p)->second;
}

/*
    Same as `World::getBlock` except it will attempt to load the block
    from the db/create an empty air block if the block is not in the cache.
*/
Block *World::getBlockForced(Point p) {
    if (blocks.count(p) != 1) {
        this->loadBlock(p);
    }

    return blocks.find(p)->second;
}

Block::Block(World *w, sqlite3_stmt *res) {
    this->world = w;

    // Load block type, if it doesnt exist in index assign it to null
    int type = sqlite3_column_int(res, 0);
    if (this->world->type_index->count(type) == 1) {
        this->type = (*this->world->type_index)[type];
    } else {
        DEBUG("Found unknown block type %i, using null", type);
        this->type = this->world->findBlockType("null");
    }

    this->pos.x = sqlite3_column_int(res, 1);
    this->pos.y = sqlite3_column_int(res, 2);
    this->pos.z = sqlite3_column_int(res, 3);
}

bool Block::save() {
    // Hitting this point means we've already (kind of) saved the block
    this->dirty = false;

    // Break out if this block doesnt persist in the world-store.
    if (!this->type->persists) {
        DEBUG("Block type %s does not persist, not saving block!",
            this->type->type.c_str());
        return false;
    }

    sqlite3_stmt *stmt = this->world->db->getCached("insert_block");

    sqlite3_bind_int(stmt, 1, this->type->id);
    sqlite3_bind_double(stmt, 2, this->pos.x);
    sqlite3_bind_double(stmt, 3, this->pos.y);
    sqlite3_bind_double(stmt, 4, this->pos.z);

    int s = sqlite3_step(stmt);

    if (s != SQLITE_DONE) {
        ERROR("Failed to save block %i!", s);
        throw Exception("Error occured while saving block to db!");
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return true;
}
