#pragma once

#include "global.h"
#include "util/geo.h"
#include "util/ioutil.h"
#include "terra/blocktype.h"
#include "terra/gen.h"

#include "rocksdb/db.h"

#include "world.pb.h"

using namespace rapidjson;

namespace Terra {

static const int MIN_SUPPORTED_WORLD_VERSION = 1;
static const int CURRENT_WORLD_VERSION = 1;

class World;
class Biome;
class Block;
class BlockType;
class WorldGenerator;

// The base block in the world
class Block {
    public:
        World *world;
        BlockType* type;
        Point pos;

        Block(World*, BlockType* type, Point);

        std::string get_meta(std::string);
        void set_meta(std::string, std::string);

        static std::string key(Point);

};

// Define a mapping to hold Point-indexed blocks
typedef std::unordered_map<Point, Block*, pointHashFunc, pointEqualsFunc> BlockCache;
typedef std::map<std::string, BlockType*> BlockTypeCache;

class World {
    private:
        FILE *fp;

    public:
        rocksdb::DB *db;
        std::string path;
        std::string name;
        uint16_t version;
        std::string seed;

        void open();
        void dump();
        void close();

        // Map block type names to id's
        std::map<std::string, uint32_t> block_type_mapping;

        WorldGenerator *gen;
        BlockTypeCache types;

        BlockCache blocks;
        std::vector<Biome *> biomes;

        World(std::string);

        void generateInitialWorld();

        // Block types
        uint32_t getNextBlockTypeID();
        uint32_t getBlockTypeID(BlockType*);
        void addBlockType(BlockType*);

        // Block based operations
        Block *get_block(Point);
        Block *create_block(Point, BlockType*);
        Block *create_block(Point, std::string);
        void save_block(Block*);
};

class Biome {
    Point a, b;

    std::string type;
};

class BlockBoundingBox : public BoundingBox {
    public:
        BlockCache blocks;

        void fill(std::string type);
        void save(World*);
};

}
