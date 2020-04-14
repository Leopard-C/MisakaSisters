#include "Client.h"
#include "Util.h"
#include <unistd.h>

#include <random>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

bool Client::shouldQuit = false;
bool Client::isQuit = false;
int Client::COUNT = 0;
int Client::MAX_COUNT_TO_SLEEP = 300;
int Client::SLEEP_TIME = 20;
int Client::SLEEP_COUNT = 0;
int Client::CURRENT_PROXY_ID = 0;
std::vector<Proxy> Client::proxies;

Client::Client()
{
    curl_ = curl_easy_init();
}

Client::~Client() {
    if (curl_)
        curl_easy_cleanup(curl_);
}


void Client::addProxy(int type, bool isNeedAuth, const std::string& addr, const std::string& userpwd) {
    proxies.emplace_back(type, isNeedAuth, addr, userpwd);
}


// 合并参数为字符串(key1=value1&key2=value2&key2=value3)
std::string Client::getMergedParam() {
    std::string mergedParam;
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it != params_.begin()) {
            mergedParam += "&";
        }
        mergedParam += it->first;
        mergedParam += "=";
        mergedParam += it->second;
    }
    return mergedParam;
}

// 发送Get请求
bool Client::Get() {
    std::string response;
    CURLcode res = curlGet_(url_, response);
    return res == CURLE_OK;
}

bool Client::Get(std::string& response) {
    CURLcode res = curlGet_(url_, response);
    if (res == CURLE_OK) {
        std::string::size_type pos;
        int offset = 0;
        if (response.find('\r') == std::string::npos) {
            pos = response.find("\n\n");
            offset = 2;
        }
        else {
            pos = response.find("\r\n\r\n");
            offset = 4;
        }
        if (pos != std::string::npos) {
            response = response.substr(pos + offset);
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}


// 发送Post请求
bool Client::Post() {
    std::string response;
    CURLcode res = curlPost_(url_, getMergedParam(), response);
    return res == CURLE_OK;
}

// URL编码
void Client::encode(std::string& str) {
    char* encodedStr = curl_easy_escape(curl_, str.c_str(), str.size());
    str = std::string(encodedStr);
    curl_free(encodedStr);
}


// curl辅助函数
size_t Client::reqReply_(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::string *str = (std::string*)stream;
    //DEBUG("---->reply");
    //DEBUG(*str);
    (*str).append((char*)ptr, size*nmemb);
    return size * nmemb;
}

// 发送Post请求(私有函数)
CURLcode Client::curlPost_(const std::string& url, const std::string& param, std::string& response) {
    checkSleep_();
    addHeader_();
    setRandomProxy_();
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str()); // url
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, param.length());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, param.c_str());
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, false); // 
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, false); // Not verify cert & host
    curl_easy_setopt(curl_, CURLOPT_POST, 1);
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl_, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, reqReply_);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);   
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 2); // 超时

    CURLcode ret = curl_easy_perform(curl_);
    curl_easy_cleanup(curl_);
    curl_ = curl_easy_init();
    return ret;
}


// 发送get请求(私有函数)
CURLcode Client::curlGet_(const std::string& url, std::string& response) {
    checkSleep_();
    addHeader_();
    setRandomProxy_();
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl_, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, reqReply_);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 2); // 超时

    CURLcode ret = curl_easy_perform(curl_);
    curl_easy_cleanup(curl_);
    curl_ = curl_easy_init();
    return ret;
}

void Client::checkSleep_() {
    Client::COUNT++;
    if (Client::COUNT > Client::MAX_COUNT_TO_SLEEP) {
        Client::COUNT = 0;
        Client::SLEEP_COUNT++;
        util::LInfoN("\nSleeping ", Client::SLEEP_TIME, "s...");
        std::this_thread::sleep_for(std::chrono::seconds(Client::SLEEP_TIME));
    }
}

void Client::setRandomProxy_() {
    if (proxies.empty())
        exit(1);
    int size = proxies.size();
    CURRENT_PROXY_ID = rand() % size;

    const Proxy& proxy = proxies[CURRENT_PROXY_ID];
    curl_easy_setopt(curl_, CURLOPT_PROXY, proxy.addr.c_str());
    curl_easy_setopt(curl_, CURLOPT_PROXYTYPE, proxy.type);
    if (proxy.isNeedAuth) {
        curl_easy_setopt(curl_, CURLOPT_PROXYUSERPWD, proxy.userpwd.c_str());
    }
}


// 添加请求头
void Client::addHeader_() {
    if (headers_.empty())
        return;

    std::string header;
    struct curl_slist* chunk = nullptr;
    for (auto it = headers_.begin(); it != headers_.end(); ++it) {
        header = it->first;
        header += ":";
        header += it->second;
        chunk = curl_slist_append(chunk, header.c_str());
    }
    //DEBUG(header);
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, chunk);
}


// Background thread:
//   Check the response time of proxy ip address every 2 min
//
/*static*/ bool Client::LoadProxyIp(const std::string& filename) {
    if (!readProxyIpFromFile(filename)) {
        return false;
    }
    if (Client::proxies.empty()) {
        return false;
    }

    // Background thread
    // Check the response time of proxy server
    std::thread t([]{

        // pid file
        // 
        char pidfile[64] = { 0 };
        sprintf(pidfile, "%s/.misaka/%d", getenv("HOME"), getpid());

        auto& proxies = Client::proxies;
        std::vector<Proxy> unUsedProxies;
        std::string cmdBase = "curl http://passport.bilibili.com/web/generic/check/"
            "nickname\?nickName\=TEMP123 -s -m 3 -x ";
        auto getResponseTime = [&cmdBase](const Proxy& proxy){
            std::string cmd = cmdBase;
            // test response-time for 3 times and get average time
            if (proxy.isNeedAuth) {
                std::string authOpt = proxy.userpwd;
                authOpt += "@";
                cmd += authOpt;
            }
            cmd += proxy.addr;
            char result[128];
            int count = 0;
            for (int i = 0; i < 3; ++i) {
                auto start = std::chrono::system_clock::now();
                memset(result, 0, 128);
                util::execShell(cmd.c_str(), result, 127);
                auto end = std::chrono::system_clock::now();
                auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                count += dur.count();
                if (count > 3000)
                    break;
            }
            return count;
        };

        while (true) {
            util::LInfoN("\nChecking response time. Proxies Count: ", proxies.size());
            util::LogToFileF(pidfile, "%d %d %d\n", time(NULL), proxies.size(),
                             Client::SLEEP_COUNT * Client::MAX_COUNT_TO_SLEEP + Client::COUNT);

            std::vector<Proxy> tmpVec;
            for (auto iter = proxies.begin(); iter != proxies.end();/* ++iter*/) {
                if (shouldQuit) {
                    util::LInfoN("Background thread quit");
                    isQuit = true;
                    return;
                }
                auto responseTime = getResponseTime(*iter);
                if (responseTime > 3000) {
                    tmpVec.emplace_back(*iter);
                    iter = proxies.erase(iter);
                    util::LErrorN("Remove proxy [", iter->addr, "] Proxies Count:", proxies.size());
                }
                else {
                    ++iter;
                }
            }

            for (auto iter = unUsedProxies.begin(); iter != unUsedProxies.end();/* ++iter*/) {
                if (shouldQuit) {
                    util::LInfoN("Background thread quit");
                    isQuit = true;
                    return;
                }
                auto responseTime = getResponseTime(*iter);
                if (responseTime > 2500) {
                    ++iter;
                }
                else {
                    proxies.emplace_back(*iter);
                    iter = unUsedProxies.erase(iter);
                    util::LInfoN("Add proxy [", iter->addr, "] Proxies Count:", proxies.size());
                }
            }
            if (tmpVec.size() > 0) {
                unUsedProxies.insert(unUsedProxies.end(), tmpVec.begin(), tmpVec.end());
            }
            for (int i = 0; i < 240; ++i) {
                if (shouldQuit) {
                    util::LInfoN("Background thread quit");
                    isQuit = true;
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        } // end while(true)
    }); // end thread t

    t.detach();
    return true;
}

/*static*/ bool Client::readProxyIpFromFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open())
        return false;

    while (!ifs.eof()) {
        std::string typeStr, addrStr, userpwdStr;
        ifs >> typeStr >> addrStr >> userpwdStr;
        if (ifs.fail())
            break;
        int type = CURLPROXY_HTTP;
        if (typeStr == "HTTPS") {
            //type == CURLPROXY_HTTPS; 
        }
        if (userpwdStr == "NULL") {
            Client::proxies.emplace_back(type, false, addrStr, "");
        }
        else {
            Client::proxies.emplace_back(type, true, addrStr, userpwdStr);
        }
    }

    ifs.close();
    return true;
}

/*static*/ void Client::Quit() {
    shouldQuit = true;
    util::LInfoN("Waiting for background thread to quit...");
    while (!isQuit) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

