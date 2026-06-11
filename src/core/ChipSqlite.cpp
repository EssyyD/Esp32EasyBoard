#include "ChipSqlite.h"
#include "EasyBoard.h"

#include <Arduino.h>

bool ChipSqlite::boot(EasyBoard* board, const char* serviceName) {
    for (auto& s : this->_bootCallers)
        if (s == serviceName) return false;
    bool first = this->_bootCallers.empty();
    if (first) {
        this->_ensureDir();

        int rc = sqlite3_initialize();
        if (rc != SQLITE_OK) {
            Serial.printf("[ChipSqlite] sqlite3_initialize FAILED: %s\n", sqlite3_errstr(rc));
            return false;
        }
        rc = sqlite3_open(this->dbFile, &this->db);
        if (rc != SQLITE_OK) {
            Serial.printf("[ChipSqlite] open FAILED: %s\n", this->db ? sqlite3_errmsg(this->db) : sqlite3_errstr(rc));
            if (this->db) sqlite3_close(this->db);
            this->db = nullptr;
            return false;
        }
        Serial.printf("[ChipSqlite] db open OK, file=%s\n", this->dbFile);
    }
    this->_bootCallers.push_back(serviceName);
    Serial.printf("[ChipSqlite] booted by '%s' (bootCount=%d)\n", serviceName, (int)this->_bootCallers.size());
    return first;
}

void ChipSqlite::unboot(const char* serviceName) {
    for (auto it = this->_bootCallers.begin(); it != this->_bootCallers.end(); ++it) {
        if (*it == serviceName) { this->_bootCallers.erase(it); break; }
    }
    if (!this->_bootCallers.empty()) {
        Serial.printf("[ChipSqlite] still has %d service(s)\n", (int)this->_bootCallers.size());
        return;
    }
    if (this->db) { sqlite3_close(this->db); this->db = nullptr; }
    sqlite3_shutdown();
    Serial.println("[ChipSqlite] closed");
}

bool ChipSqlite::exec(const char* sql) {
    if (!this->db) return false;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(this->db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        Serial.printf("[ChipSqlite] exec FAILED: %s\n", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool ChipSqlite::queryRow(const char* sql, SqliteRowCallback callback) {
    if (!this->db) return false;
    bool found = false;

    auto wrapper = [](void* arg, int colCount, char** values, char** colNames) -> int {
        auto* ctx = (bool*)arg;
        *ctx = true;
        auto* cb = (SqliteRowCallback*)(ctx + 1);
        (*cb)(colCount, values, colNames);
        return 1;
    };

    struct { bool found; SqliteRowCallback cb; } ctx = {false, callback};
    char* errMsg = nullptr;
    int rc = sqlite3_exec(this->db, sql, wrapper, &ctx, &errMsg);
    if (rc != SQLITE_OK) {
        Serial.printf("[ChipSqlite] query FAILED: %s\n", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return ctx.found;
}

bool ChipSqlite::queryAll(const char* sql, SqliteRowCallback callback) {
    if (!this->db) return false;
    int count = 0;

    auto wrapper = [](void* arg, int colCount, char** values, char** colNames) -> int {
        (*((int*)arg))++;
        auto* cb = (SqliteRowCallback*)((int*)arg + 1);
        (*cb)(colCount, values, colNames);
        return 0;
    };

    struct { int count; SqliteRowCallback cb; } ctx = {0, callback};
    char* errMsg = nullptr;
    int rc = sqlite3_exec(this->db, sql, wrapper, &ctx, &errMsg);
    if (rc != SQLITE_OK) {
        Serial.printf("[ChipSqlite] queryAll FAILED: %s\n", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool ChipSqlite::tableExists(const char* tableName) {
    if (!this->db) return false;
    String sql = String("SELECT name FROM sqlite_master WHERE type='table' AND name='") + tableName + "'";
    return this->queryRow(sql.c_str(), [](int colCount, char** values, char** colNames) {});
}

bool ChipSqlite::_ensureDir() {
    if (LittleFS.exists(this->chipSqliteRoot)) return true;
    String testFile = String(this->chipSqliteRoot) + "/.init";
    File f = LittleFS.open(testFile, "w");
    if (!f) {
        Serial.printf("[ChipSqlite] cannot create dir: %s\n", this->chipSqliteRoot);
        return false;
    }
    f.close();
    return true;
}
