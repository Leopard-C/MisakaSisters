#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <map>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <curl/easy.h>

struct Proxy {
    Proxy(int type, bool isNeedAuth, const std::string& addr, const std::string& userpwd) :
        type(type), isNeedAuth(isNeedAuth), addr(addr), userpwd(userpwd) {}
    int type;
    bool isNeedAuth;
    std::string addr;
    std::string userpwd;
};


class Client {
public:
  Client();
  ~Client();
public:
  void setUrl(const std::string& url) { url_ = url; }

  void addProxy(int type, bool isNeedAuth, const std::string& addr, const std::string& userpwd);

  void emplaceParam(const std::string& key, const std::string& value) { params_.emplace(key, value); }
  void emplaceParam(const std::string& key, int value) { params_.emplace(key, std::to_string(value)); }
  void emplaceParam(const std::string& key, int64_t value) { params_.emplace(key, std::to_string(value)); }
  void emplaceHeader(const std::string& key, const std::string& value) { headers_.emplace(key, value); }
  std::string getMergedParam();
  void clearParam() { params_.erase(params_.begin(), params_.end()); }

  void encode(std::string& str);    // URL编码
  bool Get(std::string& res);
  bool Get();
  bool Post();

private:
  CURLcode curlPost_(const std::string& url, const std::string& param, std::string& response);
  CURLcode curlGet_(const std::string &url, std::string& response);
  static size_t reqReply_(void* ptr, size_t size, size_t nmemb, void* stream);
  void addHeader_();
  void setRandomProxy_();
  void checkSleep_();

public:
  static bool LoadProxyIp(const std::string& filename);
  static bool readProxyIpFromFile(const std::string& filename);
  static void Quit();

public:
  static bool shouldQuit;
  static bool isQuit;
  static int COUNT/* = 0*/;
  static int MAX_COUNT_TO_SLEEP/* = 1000*/;  // If GET or POST 500 times, the COUNT will be set to -1.
  static int SLEEP_TIME/* = 20*/;           // 20 seconds
  static int SLEEP_COUNT;
  static int CURRENT_PROXY_ID;

  static std::vector<Proxy> proxies; // ip proxy pool

private:
  CURL* curl_;
  std::string url_;

  std::multimap<std::string, std::string> params_;
  std::multimap<std::string, std::string> headers_;
};


#endif // __CLIENT_H__
