#pragma once

#include "global.h"
#include "db.h"
#include "util.h"
#include "geo.h"
#include "entity.h"
#include "gen/world.pb.h"

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
class Chunk;

class BlockType {
    public:
        int id;

        std::string type;
        bool active;
        bool is_custom;

        std::string owner;
        bool persists = true;
        bool transparent = false;

        BlockType(std::string type_name, bool is_custom) {
            this->type = type_name;
            this->is_custom = is_custom;
        };
};


typedef std::map<int, BlockType*> BlockTypeIndexT;

void load_default_block_types(BlockTypeIndexT *);

class Block {
    public:
        int id;
        World *world;
        BlockType *type;
        Point pos;
        bool dirty = false;

        Block(World *w, sqlite3_stmt *res);

        Block(World *w, Point pos) {
            this->world = w;
            this->pos = pos;
        };

        Block(World *w, Point pos, BlockType *type) {
            this->world = w;
            this->pos = pos;
            this->type = type;
        }

        // Mark a block to be saved at SOME point
        void mark() {
            this->dirty = true;
        }

        Point getChunkPos() {
            return Point(
                this->pos.x / 16,
                this->pos.y / 16,
                this->pos.z / 16);
        }


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
typedef std::unordered_map<Point, Chunk*, pointHashFunc, pointEqualsFunc> ChunkCacheT;

class Chunk {
    public:
        Point pos;

        // True if this chunk needs to be recalculated BEFORE the next render
        bool dirty = true;

        // False if ALL sides are covered by non-transparent blocks
        bool visible = true;

        // True if there are ANY complex-type blocks in this chunk (transparent, etc)
        bool complex = false;

        BlockCacheT blocks;

        // Represents a list of 
        std::vector<Point> to_render;

        // TODO: will generate a cached texture for each side
        void generateTexture();

        // TODO: builds visible, complex, etc
        void calculate();

        Chunk(Point p) {
            this->pos = p;
        }

        void addBlock(Block *b);
};


class World {
    public:
        BlockTypeIndexT *type_index;
        std::vector<Entity *> entities;

        // Finds a block type by string, returns nullptr on no match or more than one match
        BlockType *findBlockType(std::string s);

};

class ServerWorld: public World {
    public:
        WorldFile *wf;
        DB *db;

        BlockCacheT blocks;

        // This is a queue of async loaded blocks that need to be added
        //  on the main loop.
        std::queue<Block *> blockQueue;
        std::mutex blockQueueLock;

        ServerWorld(WorldFile *wf);
        ServerWorld(std::string path);
        ~ServerWorld();

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

        // Gets a block at point P from cache or loads it and returns
        Block *getBlockForced(Point);

        // Gets a block at point P from cache or returns null
        Block *getBlock(Point);

        // Commits the blocktypeindex to the db
        bool addBlockType(BlockType *bt);

        bool saveBlock(Block *b);

};

class ClientWorld: public World {
    public:
        ChunkCacheT chunks;

        Chunk *getChunk(int, int, int, bool create=false);
        Chunk *getChunk(Point p, bool create=false);
        bool storeBlock(Block *b);
};

class Generator {
    public:
        uint seed;

        void generate(World*, PointV);
};
