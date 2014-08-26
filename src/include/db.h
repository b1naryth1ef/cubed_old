#pragma once

#include "global.h"

class DB {
    public:
        sqlite3 *db;
        std::map<std::string, std::string> tables;
        bool is_new;

        DB(std::string path);
        ~DB();

        bool create();
        void addTable(std::string, std::string, bool);
        bool dropAll();
        bool drop(std::string table);

        void begin();
        void end();
};

void init_db_module();