#pragma once

#include "db.h"

const char *ztail;

DB::DB(std::string path) {
    DEBUG("Opening SQLite table %s", path.c_str());

    this->is_new = !ioutil::file_exists(path);

    int rc = sqlite3_open(path.c_str(), &this->db);
    if (rc) {
        ERROR("SQLite Open Error: %i", rc);
        throw Exception("Failed to open database file: " + rc);
    }

    // Turn of sync-to-disk cuz fuck it
    assert(sqlite3_exec(this->db, "PRAGMA synchronous = OFF", 0, 0, 0) == SQLITE_OK);
}

DB::~DB() {
    DEBUG("Closing database...");
    sqlite3_close(db);
    DEBUG("Database closed.");
}

bool DB::create() {
    DEBUG("Creating SQLite tables...");

    int res;

    for (auto& kv : tables) {
        res = sqlite3_exec(db, kv.second.c_str(), 0, 0, 0);

        if (res != SQLITE_OK) {
            ERROR("Error creating table %s", kv.first.c_str());
            throw Exception("Error creating table!");
        }
    }
}

void DB::addTable(std::string name, std::string sql, bool create) {
    DEBUG("Adding SQLite table %s", name.c_str());

    tables[name] = sql;

    if (create) {
        int res = sqlite3_exec(db, sql.c_str(), 0, 0, 0);

        if (res != SQLITE_OK) {
            ERROR("Error creating table %s on add", name.c_str());
            throw Exception("Error creating table on add!");
        }
    }
}

bool DB::dropAll() {
    DEBUG("Dropping all SQLite tables");

    int res = sqlite3_exec(db,
        "PRAGMA writable_schema = 1;"
        "delete from sqlite_master where type = 'table';"
        "PRAGMA writable_schema = 0;"
        "VACUUM;"
        "PRAGMA INTEGRITY_CHECK;", 0, 0, 0);

    if (res != SQLITE_OK) {
        ERROR("Error dropping all tables!");
        throw Exception("Error dropping all tables!");
    }
}

void DB::begin() {
    assert(sqlite3_exec(this->db, "BEGIN TRANSACTION;", 0, 0, 0) == SQLITE_OK);
}

void DB::end() {
    assert(sqlite3_exec(this->db, "END TRANSACTION;", 0, 0, 0) == SQLITE_OK);
}

bool DB::addCached(std::string name, std::string query) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(this->db, query.c_str(), 256, &stmt, &ztail);
    this->cached[name] = stmt;
}

sqlite3_stmt *DB::getCached(std::string name) {
    return this->cached[name];
}

// Should be called on startup, makes sure SQLite is configured correctly
void init_db_module() {
    sqlite3_config(SQLITE_CONFIG_SERIALIZED);
}
