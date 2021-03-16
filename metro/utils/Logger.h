#pragma once
#include "metro/utils/LogStream.h"
#include <functional>
#include "metro/utils/Date.h"
namespace metro
{
class Logger
{
  public:
    enum LogLevel
    {
        kTrace = 0,
        kDebug = 1,
        kInfo = 2,
        kWarn = 3,
        kError = 4,
        kFatal = 5
    };
    
    class SourceFile
    {
      public:
        template<size_t N>
        explicit SourceFile(const char (&name)[N])
          : data(name), len_(N - 1)
        {
            const char *slash = strrchr(data_, '/');
            if(slash != nullptr)
            {
                data_ = slash + 1;
            }
            len_ = strlen(data_);
        }

        explicit SourceFile(const char *name)
          : data_(name)
        {
            const char *slash = strrchr(data_, '/');
            if(slash != nullptr)
            {
                data_ = slash + 1;
            }
            len_ = strlen(data_);
        }

      private:
        const char *data_;
        size_t len_;
    };

    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, bool isSysErr);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    ~Logger();
    LogStream& stream();

    static void setOutputFunction(
        std::function<void(const char*, const uint64_t len)> outputFunc,
        std::function<void()> flushFunc)
    {
        outputFunc_() = outputFunc;
        flushFunc_() = flushFunc;
    }

    static void setLogLevel(LogLevel level)
    {
        logLevel_() = level;
    }

    static LogLevel logLevel()
    {
        return logLevel_();
    }

  private:
    
    static void defaultOutputFunction(const char *msg, const uint64_t len)
    {
        fwrite(msg, 1, len, stdout);
    }
    
    static void defaultFlushFunction()
    {
        fflush(stdout);
    }

    static LogLevel &logLevel_()
    {
        static LogLevel log_level = LogLevel::kDebug;
        return log_level;
    }

    static std::function<void(const char*, const uint64_t len)> &outputFunc_()
    {
        static std::function<void(const char*, const uint64_t len)> output_function = defaultOutputFunction;
        return output_function;
    }

    static std::function<void()> &flushFunc_()
    {
        static std::function<void()> flush_function = defaultFlushFunction;
    }
    
    SourceFile sourceFile_;
    int fileLine_;
    LogLevel level_;
    LogStream logStream_;
    Date date;
};



}
