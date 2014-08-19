#pragma once

#include "global.h"
#include "db.h"

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

        bool save(DB *db);
};

typedef std::vector<Block*> BlockV;

class WorldFile {
    private:
        FILE *fp;
    public:
        DB *db;
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
    private:
        WorldFile *wf;

    public:
        std::map<Point*, Block*> blocks;
        DB *db;

        World(WorldFile *wf);

        // Checks which blocks are loaded and optionally unloads some
        int tick();

        // Rebuilds all block caches
        void recache();

        // Attempts to load a set of blocks
        bool loadBlocks(PointV points);

        // Gets a block at point P or returns null
        Block *getBlock(Point *p);
};