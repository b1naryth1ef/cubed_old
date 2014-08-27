#pragma once

#include "global.h"
#include "db.h"
#include "util.h"
#include "geo.h"
#include "entity.h"

using namespace rapidjson;

static const int SUPPORTED_WORLD_FILE_VERSION = 1;

// These should always match those in the DB, otherwise something's fucky
static const int BLOCK_TYPE_NULL = 1;
static const int BLOCK_TYPE_AIR = 2;


// Forward declare everything
class BlockType;
class Block;
class WorldFile;
class World;

class BlockType {
    public:
        int id;

        std::string type;
        std::string owner;

        bool active;
        bool is_custom;

        BlockType(std::string type_name, bool is_custom) {
            this->type = type_name;
            this->is_custom = is_custom;
        };

        BlockType(sqlite3_stmt *res);
};


typedef std::map<int, BlockType*> BlockTypeIndexT;

void load_default_block_types(BlockTypeIndexT *);

class Block {
    public:
        int id;
        World *world;
        BlockType *type;
        Point *pos;


        Block(World *w, sqlite3_stmt *res);

        Block(World *w, Point *pos) {
            this->world = w;
            this->pos = pos;
        };

        Block(World *w, Point *pos, BlockType *type) {
            this->world = w;
            this->pos = pos;
            this->type = type;
        }

        bool save();
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
        bool setupDatabase();
        bool close();

        ~WorldFile();
};

typedef std::unordered_map<Point, Block*, pointHashFunc, pointEqualsFunc> BlockCacheT;

class World {
    public:
        WorldFile *wf;

        BlockTypeIndexT *type_index;
        BlockCacheT blocks;
        std::vector<Entity *> entities;
        DB *db;

        // This is a queue of async loaded blocks that need to be added
        //  on the main loop.
        std::queue<Block *> blockQueue;

        World(WorldFile *wf);
        World(std::string path);
        ~World();

        bool load();
        bool close();
        bool tick();

        // Rebuilds all block caches
        void recache();

        bool loadBlocksAsync(PointV *, bool cleanup = true);

        // Attempts to load a set of blocks
        bool loadBlocks(PointV);

        // Attempts to load a single block
        bool loadBlock(Point, bool safe = false);

        // Gets a block at point P from cache or returns null
        Block *getBlock(Point);

        // Gets a block at point P from cache or loads it and returns
        Block *getBlockForced(Point);

        // Commits the blocktypeindex to the db
        bool addBlockType(BlockType *bt);

        // Finds a block type by string, returns nullptr on no match or more than one match
        BlockType *findBlockType(std::string s);

        bool loadBlockTypeIndex();
};
