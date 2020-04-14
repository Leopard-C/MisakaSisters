#include "MysqlDB.h"
#include "Util.h"
#include "jsoncpp/json/json.h"
#include <fstream>

MysqlDB::MysqlDB()
    : mysql(nullptr), result(nullptr)
{
    mysql = mysql_init(mysql);
    if (!mysql) {
        util::LErrorN("Mysql Init error");
    }
}

MysqlDB::~MysqlDB() {
    if (!mysql) {
        mysql_close(mysql);
    }
}

bool MysqlDB::connect(const char* host, unsigned int port, const char* user,
        const char* pwd, const char* db_name) {
    mysql = mysql_real_connect(mysql, host, user, pwd,
            db_name, port, NULL, 0); 
    if (!mysql) {
        util::LErrorF("Connect db failed: %s\n", mysql_error(mysql));
        return false;
    }
    mysql_set_character_set(mysql,"utf8");
    return true;
}

bool MysqlDB::connect(const char* dbInfoFile) {
    std::ifstream ifs(dbInfoFile);
    if (!ifs.is_open()) {
        util::LErrorF("Read dbInfoFile failed: %s\n", dbInfoFile);
        return false;
    }

    Json::Reader reader;
    Json::Value  root;
    if (!reader.parse(ifs, root, false)) {
        util::LErrorF("Parse dbInfoFile failed: %s\n", dbInfoFile);
        ifs.close();
        return false;
    }

    std::string host = root["host"].asString();
    unsigned int port = root["port"].asUInt();
    std::string user = root["user"].asString();
    std::string pwd = root["password"].asString();
    std::string dbname = root["dbname"].asString();

    ifs.close();
    return connect(host.c_str(), port, user.c_str(), pwd.c_str(), dbname.c_str());
}

bool MysqlDB::exec(const char* sql) {
    if (result) {
        mysql_free_result(result);
        result = nullptr;
    }
    if (mysql_query(mysql, sql) != 0) {
        util::LErrorF("Exec sql error: %s\n", mysql_error(mysql));
        return false;
    }
    result = mysql_store_result(mysql);
    return true;
}

