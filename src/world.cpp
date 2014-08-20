#include "world.h"

void load_default_block_types() {
    DEBUG("Loading default block types...");

    BlockType *null_bt = new BlockType("null");
    BlockTypeIndex[null_bt->type] = null_bt;

    BlockType *air_bt = new BlockType("air");
    BlockTypeIndex[air_bt->type] = air_bt;
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

BlockType::BlockType(std::string type_name) {
    this->type = type_name;
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
    err = sqlite3_exec(db->db, "CREATE TABLE blocks ("
        "id INTEGER PRIMARY KEY ASC,"
        "type TEXT,"
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
        this->loadBlock(i);
    }

    return true;
}

bool World::loadBlock(Point *p) {

    Block *b;
    sqlite3_stmt *stmt;
    char *query = (char *) malloc(2048);
    const char *ztail;

    sprintf(query, "SELECT * FROM blocks WHERE x=%F AND y=%F AND z=%F", p->x, p->y, p->z);
    int err = sqlite3_prepare_v2(db->db, query, 100, &stmt, &ztail);

    free(query);

    if (err != SQLITE_OK) {
        throw Exception("Failed to load block, sql!");
    }

    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW) {
        b = new Block(stmt);
        blocks[p] = b;
    } else if (s == SQLITE_DONE) {
        DEBUG("No block exists for %F, %F, %F! Assuming air block.", p->x, p->y, p->z);
        b = new Block(p->copy(), BlockTypeIndex["air"]);
        blocks[p] = b;
    } else {
        throw Exception("Failed to load block, query!");
    }
}

Block *World::getBlock(Point *p)  {
    if (blocks.count(p) != 1) {
        return NULL;
    }

    return blocks.find(p)->second;
}

Block::Block(sqlite3_stmt *res) {
    this->id = sqlite3_column_int(res, 0);

    const unsigned char *type;
    type = sqlite3_column_text(res, 1);

    const char *type_c = reinterpret_cast<const char*>(type);
    if (BlockTypeIndex.count(type_c) == 1) {
        this->type = BlockTypeIndex[type_c];
    } else {
        DEBUG("Found unknown block type %s, using null", type);
        this->type = BlockTypeIndex["null"];
    }
    
    this->pos->x = sqlite3_column_int(res, 2);
    this->pos->y = sqlite3_column_int(res, 3);
    this->pos->z = sqlite3_column_int(res, 4);
}
