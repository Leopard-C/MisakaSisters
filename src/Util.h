#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <vector>
#include <sstream>
#include <stdarg.h>
#include <iostream>


namespace util {

    extern char g_now[];
    extern char g_cwd[];

    // 获取当前时间
    char* getTime(const char* fmt);

    // type convert
    template <typename output_type, typename input_type>
    output_type convert(const input_type &input) {
        std::stringstream ss;
        ss << input;
        output_type result;
        ss >> result;
        return result;
    }

    // add "\" befor "'"
    std::string normalize(const std::string& str);

    // get number from string
    std::vector<int> getNumber(const std::string& str);

    // 执行shell命令并获取结果
    bool execShell(const char* cmd, char* result, int maxSize);

    // replace '0' to 'O'
    std::vector<std::string> replace0toO(const std::string& str);


    /************************** Print to console ****************************/

    // print (use std::cout)  no color, no endline
    template<typename T>
    void Log(const T& arg) {
        std::cout << arg;
    }

    template<typename T, typename ... Types>
    void Log(const T& arg1, const Types& ... args) {
        std::cout << arg1;
        Log(args...);
    }

    // print (use std::cout)  no color, with endline
    template<typename T>
    void LogN(const T& arg) {
        std::cout << arg << std::endl;
    }

    template<typename T, typename ... Types>
    void LogN(const T& arg1, const Types& ... args) {
        std::cout << arg1;
        LogN(args...);
    }

    template<typename T>
    static void LogColor_(const T& arg) {
        std::cout << arg;
    }

    template<typename T, typename ... Types>
    static void LogColor_(const T& arg1, const Types& ... args) {
        std::cout << arg1;
        Log(args...);
    }

    // print (use std::cout)  with color, no endline
    template<typename T, typename ... Types>
    void LogColor(int r, int g, int b, const T& arg1, const Types& ... args) {
        printf("\033[38;2;%d;%d;%dm", r, g, b);
        LogColor_(arg1, args...);
        printf("\033[0m");
    }

    // print (use std::cout)  with color, with endline
    template<typename T, typename ... Types>
    void LogColorN(int r, int g, int b, const T& arg1, const Types& ... args) {
        printf("\033[38;2;%d;%d;%dm", r, g, b);
        LogColor_(arg1, args...);
        printf("\033[0m\n");
    }

    void LogF(const char* fmt, ...);
    void LogColorF(int r, int g, int b, const char* fmt, ...);

    // encapsulation functions
    template<typename T, typename ... Types>
    void LError(const T& arg1, const Types& ... args)
        { LogColor(255, 0, 0, arg1, args...); }
    template<typename T, typename ... Types>
    void LInfo(const T& arg1, const Types& ... args)
        { LogColor(255, 255, 0, arg1, args...); }
    template<typename T, typename ... Types>
    void LDebug(const T& arg1, const Types& ... args)
        { Log(arg1, args...); }

    template<typename T, typename ... Types>
    void LErrorN(const T& arg1, const Types& ... args)
        { LogColorN(255, 0, 0, arg1, args...); }
    template<typename T, typename ... Types>
    void LInfoN(const T& arg1, const Types& ... args)
        { LogColorN(255, 255, 0, arg1, args...); }
    template<typename T, typename ... Types>
    void LDebugN(const T& arg1, const Types& ... args)
        { LogN(arg1, args...); }

    void LErrorF(const char* fmt, ...);
    void LInfoF(const char* fmt, ...);
    void LDebugF(const char* fmt, ...);

    void LogToFileF(const char* filename, const char* fmt, ...);
}

#endif // __UTILS_H__
