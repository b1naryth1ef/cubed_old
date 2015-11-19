#pragma once

#include "db.h"

const char *ztail;

DB::DB(std::string path) {
    DEBUG("Opening SQLite database %s", path.c_str());

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

DBQuery* DB::addCached(std::string query) {
    std::string hash = hash_string(query);

    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(this->db, query.c_str(), 256, &stmt, &ztail);

    if (result != SQLITE_OK) {
        ERROR("Failed to addCached: %i (%s)", result, sqlite3_errmsg(db));
        throw Exception("Failed to add cached SQL statement");
    }

    this->cached[hash] = new DBQuery(this, stmt);
    return this->cached[hash];
}

// Should be called on startup, makes sure SQLite is configured correctly
void DB::init_sqlite() {
    sqlite3_config(SQLITE_CONFIG_SERIALIZED);
}

DBQuery::DBQuery(DB* db, sqlite3_stmt* stmt) {
    this->db = db;
    this->stmt = stmt;
}

void DBQuery::reset() {
    this->mutex.lock();
    this->index = 1;

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
}

void DBQuery::end() {
    this->mutex.unlock();
}

int DBQuery::getLastInsertID() {
    return sqlite3_last_insert_rowid(db->db);
}

void DBQuery::bindText(std::string text) {
    sqlite3_bind_text(stmt, this->index, text.c_str(), -1, SQLITE_TRANSIENT);
    this->index++;
}

void DBQuery::bindInt(int v) {
    sqlite3_bind_int(stmt, this->index, v);
    this->index++;
}

int DBQuery::execute() {
    this->result = sqlite3_step(stmt);
    return this->result;
}

int DBQuery::getInt(int dex) {
    return sqlite3_column_int(stmt, dex);
}

std::string DBQuery::getText(int dex) {
    return std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, dex)));
}

DBQuery* DB::query(std::string sql) {
    std::string hash = hash_string(sql);

    if (cached.count(hash)) {
        return cached[hash];
    }

    DBQuery* cached = this->addCached(sql);
    cached->reset();
    return cached;
}
