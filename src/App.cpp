#include "App.h"
#include "Util.h"
#include "Validator.h"
#include "Searcher.h"
#include "User.h"

#include <iostream>
#include <string>
#include <fstream>


// Check
//  Exist or not exist.
//  Can not get detailed information.
void App::check(const char* checkListFile, bool useCache) {
    std::ifstream ifs(checkListFile);
    if (!ifs.is_open()) {
        util::LErrorF("Open file %s failed\n", checkListFile);
        return;
    }

    Validator validator;

    while (!ifs.eof()) {
        std::string name;
        ifs >> name;
        if (ifs.fail())
            break;
        auto pos = name.find("%d");
        if (pos == std::string::npos) {
            int misakaId;
            ifs >> misakaId;
            validator.check(name, misakaId, useCache);
        }
        else {
            int start, end;
            ifs >> start >> end;
            validator.check(name, start, end, useCache);
        }
    }

    validator.fixError(useCache);

    ifs.close();
}


// Search
void App::search(const char* keywordListFile) {
    std::ifstream ifs(keywordListFile);
    if (!ifs.is_open()) {
        util::LogColorF(255, 0, 0, "Open file %s failed\n", keywordListFile);
        return;
    }

    Searcher searcher;

    while (!ifs.eof()) {
        std::string keyword;
        ifs >> keyword;
        if (ifs.fail())
            break;
        auto pos = keyword.find("%d");
        if (pos == std::string::npos) {
            int misakaId;
            ifs >> misakaId;
            util::LDebugN(misakaId);
            searcher.search(keyword, misakaId);
        }
        else {
            int start, end;
            ifs >> start >> end;
            searcher.search(keyword, start, end);
        }
    }

    searcher.fixError();

    ifs.close();
}

