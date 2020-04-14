#include "Validator.h"
#include "Util.h"
#include "jsoncpp/json/json.h"
#include <iostream>

Validator::Validator() {
    if (!mysqlDB.connect("config/mysqldb.json")) {
        util::LErrorN("Connect to db failed");
        exit(1);
    }
    readAll();
}

int Validator::check(const std::string& name, int misaka_id, bool useCache) {
    if (name.find("0") == std::string::npos) {
        return check_(name, misaka_id, useCache);
    }
    else {
        std::vector<std::string> namesAll = util::replace0toO(name);
        int count = 0;
        for (auto& nm : namesAll) {
            count = check_(nm, misaka_id, useCache);
        }
        return count;
    }
}

int Validator::check_(const std::string& name, int misaka_id, bool useCache) {
    util::LogF(" [%2d] Checking: %s ", Client::CURRENT_PROXY_ID, name.c_str());

    if (useCache && nameExistCache.find(name) != nameExistCache.end()) {
        util::Log("[Exist]*\n");
        return Exist;
    }

    client.clearParam();
    std::string encodeName = name;
    client.encode(encodeName);
    client.emplaceParam("nickName", encodeName);
    std::string url = "http://passport.bilibili.com/web/generic/check/nickname?";
    url += client.getMergedParam();
    client.setUrl(url);

    std::string response;
    Json::Reader reader;
    Json::Value  root;
    int ct = 10;

    // try for 10 time at most
    // change different ip
    while (--ct) {
        if (!client.Get(response)) {
            util::LErrorN("GET Error");
            continue;
        }
        if (!reader.parse(response, root, false)) {
            util::LErrorN("Parse JSON error]");
            continue;
        }
        if (root["code"].isNull()) {
            util::LErrorN("Error: code is not 0");
            continue;
        }
        break;
    }

    if (ct == 0) {
        util::LErrorN("Error for 10 times");
        writeError(name, misaka_id); 
        return Error;
    }

    // Write to table: nicknames
    int code = root["code"].asInt();
    if (code == 40014) {
        write(name, true, misaka_id);
        util::LogN("[Exist]");
        return Exist;
    }
    else {
        write(name, false, misaka_id);
        util::LogN("[NOT Exist]");
        return NotExist;
    }
}

void Validator::check(const std::string& pattern, int start, int end, bool useCache) {
    auto pos = pattern.find("%d");
    if (pos == std::string::npos) {
        return;
    }

    std::string left = pattern.substr(0, pos);
    std::string right = pattern.substr(pos + 2);

    for (int i = start; i < end; ++i) {
        std::string name = left;
        name += std::to_string(i);
        name += right;
        check(name, i, useCache);
    }
}


void Validator::writeError(const std::string& name, int misaka_id) {
    char sql[128] = { 0 };
    sprintf(sql, "INSERT INTO nicknames_error (nickname,misaka_id) VALUES('%s',%d)"
            "ON DUPLICATE KEY UPDATE misaka_id=%d",
            name.c_str(), misaka_id, misaka_id);
    mysqlDB.exec(sql);
}


void Validator::write(const std::string& name, bool exist, int misaka_id) {
    char sql[256] = { 0 };
    sprintf(sql, "INSERT INTO nicknames (nickname, exist, misaka_id) VALUES ('%s',%d,%d)"
             "ON DUPLICATE KEY UPDATE exist=%d,misaka_id=%d;",
             util::normalize(name).c_str(), exist, misaka_id,
             exist, misaka_id);
    if (!mysqlDB.exec(sql)) {
        util::LErrorF("%s\n", sql);
        writeError(name, misaka_id);
    }
}

void Validator::readAll() {
    const char* sql = "SELECT * FROM nicknames";
    if (!mysqlDB.exec(sql)) {
        util::LErrorN("Read data from `nicknames` failed");
        return;
    }

    auto* result = mysqlDB.getResult();
    if (!result) 
        return;

    int numRows = mysql_num_rows(result);
    util::LInfoN(numRows);
    for (int i = 0; i < numRows; ++i) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row < 0)
            break;
        nameExistCache.emplace(std::string(row[0]), util::convert<bool>(row[1]));
    }
    util::LInfoN("Read ", nameExistCache.size(), " records from `nicknames`");
}

void Validator::fixError(bool useCache) {
    const char* sql = "SELECT * FROM nicknames_error";
    if (!mysqlDB.exec(sql)) {
        util::LErrorN("Read data from `nicknames_error` failed");
        return;
    }

    auto* result = mysqlDB.getResult();
    if (!result) 
        return;

    std::vector<std::pair<std::string, int>> errorRecords;
    int numRows = mysql_num_rows(result);
    if (numRows == 0)
        return;
    util::LInfoF("Fixing %d errors\n", numRows);

    for (int i = 0; i < numRows; ++i) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (!row) {
            break;
        }
        std::string name = std::string(row[0]);
        int misaka_id = util::convert<int>(row[1]);
        errorRecords.emplace_back(name, misaka_id);
    }

    for (auto pair : errorRecords) {
        if (check_(pair.first, pair.second, useCache) != -1) {
            util::LInfoN("Removing record: name=", pair.first);
            remove(pair.first);
        }
    }
}

void Validator::remove(const std::string& name) {
    char sql[64] = { 0 };
    sprintf(sql, "DELETE FROM nicknames_error WHERE name='%s'", util::normalize(name).c_str());
    if (!mysqlDB.exec(sql)) {
        util::LErrorF("%s\n", sql);
    }
}

