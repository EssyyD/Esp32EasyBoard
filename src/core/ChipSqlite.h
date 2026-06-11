#pragma once
#include <LittleFS.h>
#include <sqlite3.h>
#include <vector>
#include <functional>

class EasyBoard;

using SqliteRowCallback = std::function<void(int colCount, char** values, char** colNames)>;

class ChipSqlite {
   public:
    const char* chipSqliteRoot = "/chipSqlite";
    const char* dbFile = "/chipSqlite/chipSqlite.db";
    sqlite3* db = nullptr;

    bool boot(EasyBoard* board, const char* serviceName);
    void unboot(const char* serviceName);

    bool exec(const char* sql);
    bool queryRow(const char* sql, SqliteRowCallback callback);
    bool queryAll(const char* sql, SqliteRowCallback callback);
    bool tableExists(const char* tableName);

   private:
    std::vector<String> _bootCallers;
    bool _ensureDir();
};
