#pragma once

#include "global.h"

class DB {
    public:
        sqlite3 *db;
        DB(std::string path);
        ~DB();

};

void init_db_module();