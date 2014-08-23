#pragma once

#include "global.h"
#include "db.h"
#include "util.h"

using namespace rapidjson;

static const int SUPPORTED_WORLD_FILE_VERSION = 1;

// These should always match those in the DB, otherwise something's fucky
static const int BLOCK_TYPE_NULL = 1;
static const int BLOCK_TYPE_AIR = 2;


// Forward declare everything
class Point;
class BlockType;
class Block;
class WorldFile;
class World;

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

struct pointHashFunc {
    size_t operator()(const Point &k) const{
        size_t h1 = std::hash<double>()(k.x);
        size_t h2 = std::hash<double>()(k.y);
        size_t h3 = std::hash<double>()(k.z);
        return (h1 ^ (h2 << 1)) ^ h3;
    }
};

struct pointEqualsFunc {
  bool operator()( const Point& lhs, const Point& rhs ) const {
    return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
  }
};

typedef std::vector<Point*> PointV;

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
        bool create();
        bool close();

        ~WorldFile();
};

typedef std::unordered_map<Point, Block*, pointHashFunc, pointEqualsFunc> BlockCacheT;

class World {
    private:
        WorldFile *wf;

    public:
        BlockTypeIndexT *type_index;
        BlockCacheT blocks;
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
        // Attempts to load a single block
        bool loadBlock(Point);

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
