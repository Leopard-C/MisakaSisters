#include "Searcher.h"
#include "Util.h"
#include "jsoncpp/json/json.h"
#include <cstdlib>

Searcher::Searcher() {
    if (!mysqlDB.connect("config/mysqldb.json")) {
        util::LErrorN("Connect mysql failed");
        exit(1);
    }
    readAll();
}


// Search by keyword
int Searcher::search(const std::string& keyword, int misaka_id) {
    if (keyword.find("0") == std::string::npos) {
        return search_(keyword, misaka_id);
    }
    else {
        std::vector<std::string> keywordsAll = util::replace0toO(keyword);
        int count = 0;
        for (auto& kw : keywordsAll) {
            count = search_(kw, misaka_id);
        }
        return count;
    }
}

// Do real_searching 
int Searcher::search_(const std::string& keyword, int misakaId) {
    int count = 0;
    int i = 0;
    int pages = INT_MAX;
    while ((i++) < pages) {
        util::LogF("[%2d] keyword=\"%s\"  page=%d\n", Client::CURRENT_PROXY_ID,
                keyword.c_str(), i);
        client.clearParam();
        client.emplaceParam("search_type", "bili_user");
        std::string encodedKeyword = keyword;
        client.encode(encodedKeyword);
        client.emplaceParam("keyword", encodedKeyword);
        client.emplaceParam("page", std::to_string(i));
        std::string url = "http://api.bilibili.com/x/web-interface/search/type?";
        url += client.getMergedParam();
        client.setUrl(url);

        std::string response;
        Json::Reader reader;
        Json::Value  root;
        int ct = 10;

        // try for 10 time at most
        while (--ct) {
            if (!client.Get(response)) {
                util::LErrorN("GET Error");
                continue;
            }
            if (!reader.parse(response, root, false)) {
                util::LErrorN("Parse JSON Error");
                continue;
            }
            if (root["code"].asInt() != 0) {
                util::LErrorN("Ret code is not 0");
                continue;
            }
            break;
        }

        if (ct == 0) {
            util::LErrorN("Error for 10 times");
            writeError(keyword, misakaId);
            return -1;
        }

        Json::Value& data = root["data"];
        pages = data["numPages"].asInt();
        Json::Value& result = data["result"];
        int pageSize = result.size();
        count += pageSize;

        // 20 items at most in one page
        for (int k = 0; k < pageSize; ++k) {
            Json::Value& user = result[k];
            int64_t mid = user["mid"].asInt64();
            util::LogF("     mid=%-11lld ", mid);
            // already added to database 
            if (midsCache.find(mid) != midsCache.end()) {
                util::LogN("[Exist]*");
            }
            else {
                util::LogN("[New]");
            }
            midsCache.emplace(mid);
            std::string name = user["uname"].asString();
            std::string sign = user["usign"].asString();
            int gender = user["gender"].asInt();
            std::string face = "http:";
            std::string face_ = user["upic"].asString();
            face += face_;
            int level = user["level"].asInt();
            int fans = user["fans"].asInt();
            int videos = user["videos"].asInt();
            write(mid, name, gender, face, sign, level, misakaId, fans, videos);
        }
    }

    return count;
}


// Search continuously
// For example:
//    misaka100, misaka101, misaka102, ... misaka999
int Searcher::search(const std::string& keywordPattern, int start, int end) {
    auto pos = keywordPattern.find("%d");
    if (pos == std::string::npos) {
        return 0;
    }

    std::string left = keywordPattern.substr(0, pos);
    std::string right = keywordPattern.substr(pos + 2);
    int count = 0;

    for (int i = start; i < end; ++i) {
        std::string keyword = left;
        keyword += std::to_string(i);
        keyword += right;
        count += search(keyword, i);
    }

    return count;
}


void Searcher::fixError() {
    const char* sql = "SELECT * FROM misaka_sisters_info_error";
   if (!mysqlDB.exec(sql)) {
       util::LErrorN("Read data from `misaka_sisters_info_error` failed");
        return;
    }

    auto* result = mysqlDB.getResult();
    if (!result)  {
        util::LErrorN("Result is NULL");
        return;
    }

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
        std::string keyword = std::string(row[0]);
        int misakaId = util::convert<int>(row[1]);
        errorRecords.emplace_back(keyword, misakaId);
    }

    for (auto& pair : errorRecords) {
        if (search_(pair.first, pair.second) != -1) {
            util::LInfoN("Removing record: keyword=", pair.first);
            remove(pair.first); 
        }
    }
}


void Searcher::writeError(const std::string& name, int misaka_id) {
    char sql[128];
    sprintf(sql, "INSERT INTO misaka_sisters_info_error VALUES('%s', %d);",
            name.c_str(), misaka_id);
    mysqlDB.exec(sql);
}

void Searcher::write(int64_t mid, const std::string& name, int gender,
        const std::string& face, const std::string& sign, int level,
        int misaka_id, int fans, int videos)
{
    int exact = 1;
    if (misaka_id == -1) {
        // First: Get misaka_id from database
        char sql_query[128] = { 0 };
        sprintf(sql_query, "SELECT `name`,misaka_id,exact FROM misaka_sisters_info WHERE mid=%lld", mid);
        if (!mysqlDB.exec(sql_query)) {
            util::LogF("%s\n", sql_query);
            return;
        }
        auto* result = mysqlDB.getResult();
        if (!result) {
            util::LErrorN("result is NULL");
            return;
        }
        int numRows = mysql_num_rows(result);
        // not found in database
        if (numRows == 0) {
            // Calculate proper misaka_id
            misaka_id = getProperMisakaId(name);
            if (misaka_id != -1)
                exact = 0;    // misaka_id is not exact, need manually to verfiy 
        }
        // found in database
        else if (numRows == 1) {
            MYSQL_ROW row = mysql_fetch_row(result);
            misaka_id = util::convert<int>(row[1]);
            exact = util::convert<int>(row[2]);
            //util::LInfoF("Found: misaka_id=%d\n", misaka_id);
        }
        // won't get down here
        else {
            return;
        }
    }

    // Insert into database
    // or
    // Update exist record
    char sql[2048] = { 0 };
    std::string n_name = util::normalize(name);
    std::string n_sign = util::normalize(sign);
    sprintf(sql, "INSERT INTO misaka_sisters_info (mid,`name`,gender,face,`sign`,`level`,"
            "misaka_id,fans,videos,exact) VALUES(%lu,'%s',%d,'%s','%s',%d,%d,%d,%d,%d) "
            "ON DUPLICATE KEY UPDATE `name`='%s',gender=%d,face='%s',`sign`='%s',level=%d,"
            "misaka_id=%d,fans=%d,videos=%d,exact=%d;", 
            mid, n_name.c_str(), gender, face.c_str(), n_sign.c_str(), level, misaka_id, fans, videos, exact,
                 n_name.c_str(), gender, face.c_str(), n_sign.c_str(), level, misaka_id, fans, videos, exact);
    if (!mysqlDB.exec(sql)) {
        util::LogF("%s\n", sql);
    }
}

void Searcher::remove(const std::string& keyword) {
    char sql[128] = { 0 };
    sprintf(sql, "DELETE FROM misaka_sisters_info_error WHERE keyword='%s'", util::normalize(keyword).c_str());
    if (!mysqlDB.exec(sql)) {
        util::LogF("%s\n", sql);
    }
}

void Searcher::readAll() {
    const char* sql = "SELECT mid FROM misaka_sisters_info;";
    if (!mysqlDB.exec(sql)) {
        util::LErrorN("Read data from mysql failed");
        return;
    }

    auto* result = mysqlDB.getResult();
    if (!result) 
        return;

    int numRows = mysql_num_rows(result);
    for (int i = 0; i < numRows; ++i) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row < 0)
            break;
        int64_t mid = util::convert<int64_t>(row[0]);
        midsCache.emplace(mid);
    }
    util::LInfoF("Read %d records from `misaka_sisters_info`\n", midsCache.size());
}


int Searcher::getProperMisakaId(const std::string& name) {
    auto pos1 = name.find("O");
    auto pos2 = name.find("O");
    if (pos1 == std::string::npos && pos2 == std::string::npos) {
        std::vector<int> nums = util::getNumber(name);
        if (nums.size() == 1) {
            return nums[0];
        }
    }
    return -1;
}

void Searcher::exportToFile(const char* filename) {
    char sql[2048] = { 0 };
    sprintf(sql, 
            "SELECT * INTO OUTFILE '/var/lib/mysql-files/%s' "
            "FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' FROM "
            "(SELECT 'mid','name','misaka_id','exact','level','sign','gender','face','fans','videos' union "
            "SELECT `mid`,`name`,`misaka_id`,`exact`,`level`,`sign`,`gender`,`face`,`fans`,`videos` "
            "FROM misaka_sisters_info) b;", filename);
    if (!mysqlDB.exec(sql)) {
        util::LErrorN("Export to file failed!");
    }
}

