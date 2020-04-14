#include "User.h"

bool User::getInfo(int64_t mid, std::string& response) {
    client.clearParam();
    client.emplaceParam("mid", mid);
    client.emplaceParam("jsonp", "jsonp");
    std::string url = "https://api.bilibili.com/x/space/acc/info?";
    url += client.getMergedParam();
    client.setUrl(url);

    return client.Get(response);
}
