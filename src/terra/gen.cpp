#include "terra/gen.h"

namespace Terra {


void FlatGenerator::generate_area(BoundingBox b) {
    if (b.min.y == 0) {
        Block *blk;
        for (int x = b.min.x; x <= b.max.x; x++) {
            for (int z = b.min.z; z <= b.max.z; z++) {
                blk = this->world->create_block(Point(x, 0, z), "base:air");
                this->world->save_block(blk);
            }
        }
    }
}

}
