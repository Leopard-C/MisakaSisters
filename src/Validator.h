#pragma once

#include <map>
#include <string>
#include "Client.h"
#include "MysqlDB.h"

class Validator {
public:
    Validator();

    int check(const std::string& name, int misaka_id, bool useCache);
    void check(const std::string& pattern, int start, int end, bool useCache);

    void fixError(bool useCache);

    enum {
        Error = -1,
        NotExist = 0,
        Exist = 1,
    };

private:
    int check_(const std::string& name, int misaka_id, bool useCache);

    void write(const std::string& name, bool exist, int misaka_id);
    void writeError(const std::string& name, int misaka_id);

    void remove(const std::string& name);
    void readAll();

private:
    Client client;
    MysqlDB mysqlDB;
    std::map<std::string, bool> nameExistCache;
};

