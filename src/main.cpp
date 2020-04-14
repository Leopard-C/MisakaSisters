#include "App.h"
#include "Client.h"
#include "Util.h"
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <unistd.h>
#include "Searcher.h"

void printUsage() {
    util::LogN("Usage:");
    util::LogN("\t misaka_sisetrs [command] [param1] [para2] [param...] [proxy_list_file_name]");
    util::LogN("For example:");
    util::LogN("\t1. misaka_sisetrs check check.list USE_CACHE proxy.list");
    util::LogN("\t2. misaka_sisetrs check check.list NO_CACHE proxy.list");
    util::LogN("\t3. misaka_sisetrs search keyword.list proxy.list");
}

int main(int argc, char** argv) {
    if (argc < 4) {
        printUsage();
        return 1;
    }

    using namespace std::chrono;
    auto start = system_clock::now();
    srand(time(NULL));

    char pidfile[64] = { 0 };
    sprintf(pidfile, "%s/.misaka/%d", getenv("HOME"), getpid());
    util::LogToFileF(pidfile, "%s %s\n", argv[1], argv[2]);
    util::LogToFileF(pidfile, "START\n");

    if (!Client::LoadProxyIp(argv[argc - 1])) {
        util::LErrorF("Load proxy ip failed\n");
        util::LogToFileF(pidfile, "Load proxy ip failed\n");
        return 1;
    }

    App app;

    if (strcmp(argv[1], "check") == 0) {
        if (argc != 5) {
            printUsage();
            return 1;
        }
        if (strcmp(argv[3], "USE_CACHE") == 0 || strcmp(argv[3], "1") == 0) {
            app.check(argv[2], true);
        }
        else if (strcmp(argv[3], "NO_CACHE") == 0 || strcmp(argv[3], "0") == 0) {
            app.check(argv[2], false);
        }
    }
    else if (strcmp(argv[1], "search") == 0) {
        app.search(argv[2]);
    }
    else {
        util::LErrorF("Unknown command: %s\n", argv[1]);
    }

    Client::Quit();
    auto end = system_clock::now();
    auto duration = duration_cast<minutes>(end - start);
    util::LogN("Time consuming: ", duration.count(), "min");
    util::LogColorF(0, 255, 0, "Quit successfully!\n");
    util::LogToFileF(pidfile, "QUIT\n");
    return 0;
}

