#include "world.h"

void load_default_block_types(World *w, BlockTypeIndexT *bti) {
    DEBUG("Loading default block types...");

    BlockType *null_bt = new BlockType("null", false);
    w->addBlockType(null_bt);
    // (*bti)[null_bt->type] = null_bt;

    BlockType *air_bt = new BlockType("air", false);
    w->addBlockType(air_bt);
    // (*bti)[air_bt->type] = air_bt;
}

Point::Point(double x, double y, double z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

Point::Point(int x, int y, int z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

Point::Point(const Value &v) {
    x = v[SizeType(0)].GetInt();
    y = v[SizeType(1)].GetInt();
    z = v[SizeType(2)].GetInt();
}

BlockType::BlockType(sqlite3_stmt *res) {
    this->id = sqlite3_column_int(res, 0);
    this->type = (char *)sqlite3_column_text(res, 1);
    this->owner = (char *)sqlite3_column_text(res, 2);
    this->active = sqlite3_column_int(res, 3);
}

WorldFile::WorldFile(std::string dir) {
    directory = dir;
}

WorldFile::~WorldFile() {
    close();
}

bool WorldFile::open() {
    DEBUG("Attempting to open worldfile...");
    fp = fopen(ioutil::join(this->directory, "world.json").c_str(), "r");

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

    // Open the DB, optionally creating it for the first time if it doesnt exist
    std::string dbpath = ioutil::join(this->directory, "data.db");
    bool createdb = !ioutil::file_exists(dbpath);

    this->db = new DB(dbpath);

    if (createdb) {
        DEBUG("Creating database for first time...");
        create();
    }
}

bool WorldFile::create() {
    // First create blocks database
    int err;
    err = sqlite3_exec(db->db, "PRAGMA synchronous = OFF", 0, 0, 0);
    // err = sqlite3_exec(db->db, "PRAGMA journal_mode = MEMORY", 0, 0, 0);

    err = sqlite3_exec(db->db, "CREATE TABLE blocktypes ("
        "id INTEGER PRIMARY KEY ASC,"
        "type TEXT,"
        "owner TEXT,"
        "active INTEGER"
    ");", 0, 0, 0);

    err = sqlite3_exec(db->db, "CREATE TABLE blocks ("
        "id INTEGER PRIMARY KEY ASC,"
        "type INTEGER,"
        "x INTEGER,"
        "y INTEGER,"
        "z INTEGER"
    ");", 0, 0, 0);

    err = sqlite3_exec(db->db, "CREATE INDEX blocks_full_coord ON blocks ("
        "x ASC, y ASC, z ASC"
    ");", 0, 0, 0);

    if (err != SQLITE_OK) {
        throw Exception("Error creating first-time world database...");
    }
}

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

World::World(WorldFile *wf) {
    this->wf = wf;
    this->db = wf->db;
}

World::World(std::string path) {
    this->wf = new WorldFile(path);
    this->wf->open();
    this->db = this->wf->db;
}

bool World::load() {
    DEBUG("Loading world...");

    this->type_index = new BlockTypeIndexT();
    this->loadBlockTypeIndex();
    load_default_block_types(this, this->type_index);

    Timer t = Timer();
    t.start();

    sqlite3_exec(db->db, "BEGIN TRANSACTION", 0, 0, 0);
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            for (int z = 0; z < 32; z++) {
                this->loadBlock(Point(x, y, z));
            }
        }
    }
    sqlite3_exec(db->db, "END TRANSACTION", 0, 0, 0);

    DEBUG("Loading initial block set took %ims", t.end());

}

World::~World() {
    this->close();
}

bool World::close() {
    this->wf->close();
    return true;
}

bool World::loadBlocks(PointV points) {
    for (auto i : points) {
        this->loadBlock(*i);
    }

    return true;
}

/*
    Attempts to load a block at Point p. If the block exists in the cache,
    it will NOT be loaded and this will return false. If the block does not
    exist at all, a new air block will be created and added to BOTH the
    database and block cache.
*/
bool World::loadBlock(Point p) {
    // If the block already exists, skip this
    if (blocks.count(p)) {
        return false;
    }

    Block *b;
    sqlite3_stmt *stmt;
    char *query = (char *) malloc(2048);
    const char *ztail;

    sprintf(query, "SELECT * FROM blocks WHERE x=%F AND y=%F AND z=%F", p.x, p.y, p.z);
    int err = sqlite3_prepare_v2(db->db, query, 100, &stmt, &ztail);

    free(query);

    if (err != SQLITE_OK) {
        throw Exception("Failed to load block, sql!");
    }

    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW) {
        b = new Block(this, stmt);
        blocks[p] = b;
    } else if (s == SQLITE_DONE) {
        DEBUG("No block exists for %F, %F, %F! Assuming air block.", p.x, p.y, p.z);
        b = new Block(this, p.copy(), this->findBlockType("air"));
        b->world = this;
        blocks[p] = b;
        b->save();
    } else {
        throw Exception("Failed to load block, query!");
    }

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

bool World::addBlockType(BlockType *bt) {
    if (this->findBlockType(bt->type) != nullptr) {
        DEBUG("Cannot addBlockType on `%s`, already exists in index.", bt->type.c_str());
        return false;
    }

    this->db->begin();

    // Add to type_index
    (*this->type_index)[bt->id] = bt;

    char *query = (char *) malloc(2048);
    sprintf(query,
        "INSERT INTO blocktypes (type, owner, active)"
        "VALUES ('%s', 'test', 1);",
        bt->type.c_str());

    int res = sqlite3_exec(this->db->db, query, 0, 0, 0);
    free(query);

    int id = sqlite3_last_insert_rowid(this->db->db);
    
    this->db->end();

    if (res != SQLITE_OK) {
        ERROR("Failed to save blocktype!");
        throw Exception("Failed to save blocktype!");
    }

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
    this->id = sqlite3_column_int(res, 0);

    int type = sqlite3_column_int(res, 1);

    // const char *type_c = reinterpret_cast<const char*>(type);
    // if (this->world->type_index->count(type_c) == 1) {
    //     this->type = (*this->world->type_index)[type_c];
    // } else {
    //     DEBUG("Found unknown block type %s, using null", type);
    //     this->type = (*this->world->type_index)["null"];
    // }
    
    this->pos = new Point();
    this->pos->x = sqlite3_column_int(res, 2);
    this->pos->y = sqlite3_column_int(res, 3);
    this->pos->z = sqlite3_column_int(res, 4);
}

bool Block::save() {
    char *query = (char *) malloc(2048);

    sprintf(query,
        "INSERT INTO blocks (type, x, y, z)"
        "VALUES ('%i', %F, %F, %F);",
        this->type->id,
        this->pos->x, this->pos->y, this->pos->z
    );

    int s = sqlite3_exec(this->world->db->db, query, 0, 0, 0);
    free(query);

    if (s != SQLITE_OK) {
        ERROR("Failed to save block!");
        throw Exception("Error occured while saving block to db!");
    }

    return true;
}
