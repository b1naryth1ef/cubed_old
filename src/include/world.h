#pragma once

#include "global.h"
#include "db.h"

using namespace rapidjson;

static const int SUPPORTED_WORLD_FILE_VERSION = 1;

class Point {
    public:
        double x, y, z;

        Point(double x, double y, double z);
        Point(int x, int y, int z);
        Point(const Value &v);
        Point() {};

        Point *copy() {
            return new Point(x, y, z);
        }

        std::string debug() {
            char *x;
            sprintf(x, "Point<%F, %F, %F>", x, y, z);
            std::string result = std::string(x);
            free(x);
            return result;
        }
};

typedef std::vector<Point*> PointV;

class BlockType {
    public:
        std::string type;

        BlockType(std::string);
};

static std::map<std::string, BlockType*> BlockTypeIndex;

void load_default_block_types();

static void init_world_module() {
    load_default_block_types();
}

class Block {
    public:
        int id;
        BlockType *type;
        Point *pos;

        Block(sqlite3_stmt *res);

        Block(Point *pos) {
            pos = pos;
        };

        Block(Point *pos, BlockType *type) {
            pos = pos;
            type = type;
        }

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
        Point *origin;
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
        World(std::string path);
        ~World();

        bool load();
        bool close();

        // Checks which blocks are loaded and optionally unloads some
        int tick();

        // Rebuilds all block caches
        void recache();

        // Attempts to load a set of blocks
        bool loadBlocks(PointV);
        bool loadBlock(Point *);
        bool loadBlockForce(Point *);

        // Gets a block at point P or returns null
        Block *getBlock(Point *);
};