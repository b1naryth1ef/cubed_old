#pragma once

#include "global.h"

class DB {
    public:
        sqlite3 *db;
        std::map<std::string, std::string> tables;

        DB(std::string path);
        ~DB();

        bool create();
        void add_table(std::string, std::string, bool);
        bool drop_all();
        bool drop(std::string table);

        void begin();
        void end();
};

void init_db_module();