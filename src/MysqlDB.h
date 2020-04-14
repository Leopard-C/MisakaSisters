#pragma once

#include <mysql/mysql.h>


class MysqlDB {
public:
    MysqlDB();
    ~MysqlDB();

    bool connect(const char* host, unsigned int port, const char* user,
            const char* pwd, const char* db_name);
    bool connect(const char* dbInfoFile);
    bool exec(const char* sql);

    MYSQL_RES* getResult() const { return result; }

private:
    MYSQL* mysql;
    MYSQL_RES* result;
    MYSQL_ROW row;
};
