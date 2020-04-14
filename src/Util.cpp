#include "Util.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace util {

char g_now[32] = { 0 };
char g_cwd[256] = { 0 };

void LErrorF(const char* fmt, ...) {
    printf("\033[38;2;255;0;0m");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\033[0m");
}

void LInfoF(const char* fmt, ...) {
    printf("\033[38;2;255;255;0m");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\033[0m");
}

void LDebugF(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void LogF(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void LogColorF(int r, int g, int b, const char* fmt, ...) {
    printf("\033[38;2;%d;%d;%dm", r, g, b);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\033[0m");
}

void LogToFileF(const char* filename, const char* fmt, ...) {
    FILE* fp = fopen(filename, "a+");
    if (!fp)
        return;
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fclose(fp);
}

// 获取当前时间
char* getTime(const char* fmt) {
    time_t now = time(0);
    strftime(g_now, sizeof(g_now), fmt, localtime(&now));

    return g_now;
}

static void normalize_(const std::string& ch, std::string& str) {
    auto pos = str.find(ch);
    while (pos != std::string::npos) {
        str.insert(pos, "\\");
        pos = str.find(ch, pos + 2);
    }
}

// add "\" befor "'"
std::string normalize(const std::string& str) {
    std::string ret = str;
    normalize_("\\", ret);
    normalize_("'", ret);
    normalize_("%", ret);
    normalize_("<", ret);
    normalize_(">", ret);
    return ret;
}

static int countStartedOne(char c) {
    int bit1 = 1;
    int count = 0;
    for (int i = sizeof(c) * 8 - 1; i >= 0; i--){
        unsigned int x = (((bit1 << i)&c) != 0);
        if (x == 1)
            count++;
        else
            return count;
    }
}


// get number from string
std::vector<int> getNumber(const std::string& str) {
    int len = str.length();
    std::vector<int> nums;
    bool isNumStarted = false;
    int start = 0, end = 0;
    for (int i = 0; i < len; /*++i*/) {
        char c = str[i];
        if (c >= '0' && c <= '9') {
            if (!isNumStarted) {
                isNumStarted = true;
                start = i;
            }
            i++;
        }
        else {
            if (isNumStarted) {
                isNumStarted = false;
                end = i;
                nums.push_back(util::convert<int>(str.substr(start, end - start)));
            }
            if (c < 0) {
                i += countStartedOne(str[i]);
            }
            else {
                i++;
            }
        }
    }

    if (isNumStarted) {
        isNumStarted = false;
        end = len;
        nums.push_back(util::convert<int>(str.substr(start, end - start)));
    }

    return nums;
}


// 执行shell命令并获取输出
bool execShell(const char* cmd, char* result, int maxSize) {
    FILE* fp = popen(cmd, "r");
    if (!fp)
        return false;

    char buf[1024] = { 0 };
    while (fgets(buf, 1024, fp) != NULL) {
        if (strlen(result) + 1024 <= maxSize)
            strcat(result, buf);
        else
            strncat(result, buf, maxSize - strlen(result));
        memset(buf, 0, 1024);
    }
    pclose(fp);
    fp = NULL;

    return true;
}

// replace '0' to 'O'
// for example:
//   input: str = "misaka10032"
//  output:   misaka10032 misaka1O032 misaka10O32 misaka1OO32
std::vector<std::string> replace0toO(const std::string& str) {
    std::vector<std::string> ret;
    // find all '0'
    std::vector<int> positions;
    std::string::size_type pos = -1;
    // replace "0" to " "O"
    while ((pos = str.find("0", pos + 1)) != std::string::npos) {
        positions.push_back(pos);
    }
    int count = positions.size();
    // permutation
    for (int i = 0; i < count + 1; ++i) {
        std::vector<std::string> chars;
        for (int j = 0; j < i ; ++j)
            chars.push_back("0");
        for (int j = i; j < count; ++j)
            chars.push_back("O");
        do {
            std::string strNew = str;
            for (int k = 0; k < count; ++k) {
                strNew.replace(positions[k], 1, chars[k]);
            }
            ret.push_back(strNew);
        } while (next_permutation(chars.begin(), chars.end()));
    }

    return ret;
}

} // namespace util

