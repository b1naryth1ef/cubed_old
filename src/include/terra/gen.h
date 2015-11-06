#pragma once

#include "global.h"
#include "terra/terra.h"
#include "util/geo.h"
#include "util/channel.h"

namespace Terra {

class World;
class WorldGenerator;

class WorldGeneratorThread {
    private:
        WorldGenerator *gen;
        Channel<BoundingBox*> *q;
    public:
        std::thread *thread;
        WorldGeneratorThread(WorldGenerator*, Channel<BoundingBox*>*);

        void run();
};

class WorldGenerator {
    private:

        const uint8_t max_threads = 8;
    public:
        std::string seed;
        World *world;

        virtual void generate_block(Point) = 0;
        virtual void generate_area(BoundingBox);
};

class FlatGenerator : public WorldGenerator {
    public:
        FlatGenerator(std::string seed, World* w) {
            this->seed = seed;
            this->world = w;
        }

        void generate_block(Point);
};

}
