#pragma once

#include <set>
#include <string>
#include "Client.h"
#include "MysqlDB.h"

class Searcher {
public:
    Searcher();

    int search(const std::string& keyword, int misakaId);
    int search(const std::string& keywordPattern, int start, int end);

    void fixError();

    void exportToFile(const char* filename);

    enum {
        Error = -1,
        NotExist = 0,
        Exist = 1,
    };

private:
    int search_(const std::string& keyword, int misakaId);

    void writeError(const std::string& name, int misaka_id);
    void write(int64_t mid, const std::string& name, int gender,
            const std::string& face, const std::string& sign, int level,
            int misaka_id, int fans, int videos);

    // Remove record from database
    void remove(const std::string& keyword);

    // Read all records from database and cache them
    void readAll();

    // Get proper misaka_id
    // For example:
    //  misaka156hahaha  => 156
    //  lv6_misaka20001  => -1 (two number in the name)
    int getProperMisakaId(const std::string& name);

private:
    Client client;
    MysqlDB mysqlDB;
    std::set<int64_t> midsCache; // member id
};

