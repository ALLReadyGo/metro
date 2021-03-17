#include <iostream>
#include "metro/utils/Logger.h"
int main(int argc, char const *argv[])
{
    // g++ -g metro/utils/Date.cc metro/utils/Logger.cc metro/utils/LogStream.cc metro/tests/main.cc -I./
    using namespace metro;
    LOG_ERROR << "zhangsan";
    LOG_FATAL << "wangwu";
    LOG_SYSERR << "lisi";
    LOG_WARN << "santong";
    LOG_FATAL << "lala";

    return 0;
}
    