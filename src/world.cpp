#include "world.h"

WorldFile::WorldFile(std::string dir) {
    directory = dir;
}

WorldFile::~WorldFile() {
    close();
}

bool WorldFile::open() {
    fp = fopen(ioutil::join(this->directory, "world.json").c_str(), "r");

    if (!fp) {
        LOG.L("Failed to open world file!");
        throw Exception("Failed to open world file!");
    }

    // Create a lock for the file, which warns other instances of cubed to not write to
    //  this world file.
    int lock_result = flock(fileno(fp), LOCK_EX | LOCK_NB);
    if (lock_result != 0) {
        LOG.L("Error getting lock: %i", lock_result);

        throw Exception("Error occured getting lock for world file!");
    }

    // Read the entire json document
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;

    try {
        d.ParseStream(is);
    } catch (std::string e) {
        throw Exception("Error occured parsing json world file!");
    }

    // Parse the name and version
    name = d["name"].GetString();
    version = d["version"].GetInt();
    LOG.L("Name and version: %s, %i", name.c_str(), version);

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
        LOG.L("Creating database for first time...");
        create();
    }
}

bool WorldFile::create() {
    // First create blocks database
    int err;
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
    LOG.L("Closing world file %s", name.c_str());
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    LOG.L("Closing db...");
    delete(db);

    LOG.L("Done closing worldfile!");

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

World::~World() {
    this->close();
}

bool World::close() {
    this->wf->close();
    return true;
}

bool World::loadBlocks(PointV points) {
    const char *ztail;
    int err;

    for (auto i : points) {
        sqlite3_stmt *stmt;
        char *query;
        sprintf(query, "SELECT * FROM blocks where x=%F AND y=%F AND z=%F", i->x, i->y, i->z);
        err = sqlite3_prepare_v2(db->db, query, 100, &stmt, &ztail);

        if (err != SQLITE_OK) {
            throw Exception("Failed to load block!");
        }

        int s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
            Block b = Block(stmt);
            blocks[i] = &b;
        } else {
            throw Exception("Failed to load block!");
        }
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
    this->type = sqlite3_column_int(res, 1);
    this->pos.x = sqlite3_column_int(res, 2);
    this->pos.y = sqlite3_column_int(res, 3);
    this->pos.z = sqlite3_column_int(res, 4);
}

