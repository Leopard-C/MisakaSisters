#pragma once

#include "Client.h"


class User {
public:
    bool getInfo(int64_t mid, std::string& res);

private:
    Client client;
};
