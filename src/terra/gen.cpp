#include "terra/gen.h"

namespace Terra {

WorldGeneratorThread::WorldGeneratorThread(WorldGenerator *gen, Channel<BoundingBox*> *q) {
    this->gen = gen;
    this->q = q;

    this->thread = new std::thread([this]() {
        this->run();
    });
}

void WorldGeneratorThread::run() {
    BoundingBox *b;

    while (!q->closed) {
        b = this->q->get();
        if (b == NULL) {
            return;
        }

        for (int x = b->min.x; x < b->max.x; x++) {
            for (int y = b->min.y; y < b->max.y; y++) {
                for (int z = b->min.z; z < b->max.z; z++) {
                    this->gen->generate_block(Point(x, y, z));
                }
            }
        }

        delete(b);
    }
}

void WorldGenerator::generate_area(BoundingBox b) {
    int num_threads = this->max_threads;
    auto q = new Channel<BoundingBox*>;
    std::vector<WorldGeneratorThread*> threads;

    // Calculate the actual number of threads we need
    if (b.sizeY() < this->max_threads) {
        num_threads = b.sizeY();
    }

    for (double y = b.min.y; y <= b.max.y; y++) {
        q->put(new BoundingBox(
            Point(b.min.x, y - 1, b.min.z),
            Point(b.max.x, y, b.max.z)));
    }

    INFO("[GEN] Spawning %i threads...", num_threads);
    for (int i = 0; i < num_threads; i ++) {
        threads.push_back(new WorldGeneratorThread(this, q));

        // Put a NULL in so we close every thread when we are done
        q->put(NULL);
    }

    // Finally wait for the threads to join
    for (auto &thread : threads) {
        thread->thread->join();
    }
}

void FlatGenerator::generate_block(Point p) {
    if (p.y == 0) {
        this->world->save_block(this->world->create_block(p, "base:bedrock"));
    }
}

}
