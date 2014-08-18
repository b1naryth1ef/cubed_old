#pragma once

#include "global.h"

using namespace rapidjson;

static const int SUPPORTED_WORLD_FILE_VERSION = 1;

class Point {
    public:
        double x, y, z;
};

typedef std::vector<Point*> PointV;

class Block {
    public:
        int id, type;
        Point pos;

        Block(sqlite3_stmt *res);

        bool save(sqlite3 *db);
};

typedef std::vector<Block*> BlockV;

class WorldFile {
    private:
        FILE *fp;
    public:
        sqlite3 *db;
        std::string directory;
        std::string name;
        int version;

        WorldFile(std::string dir);

        bool open();
        bool create();
        bool close();

        ~WorldFile();
};

class World {
    public:
        std::map<Point*, Block*> blocks;
        sqlite3 *db;

        // Checks which blocks are loaded and optionally unloads some
        int tick();

        // Rebuilds all block caches
        void recache();

        // Attempts to load a set of blocks
        bool loadBlocks(PointV points);

        // Gets a block at point P or returns null
        Block *getBlock(Point *p);
};