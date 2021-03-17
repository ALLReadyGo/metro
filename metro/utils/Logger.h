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
        kDebug,
        kInfo,
        kWarn,
        kError,
        kFatal,
        kNumberOfLogLevels
    };
    
    class SourceFile
    {
      public:
        template<size_t N>
        SourceFile(const char (&name)[N])
          : data_(name), len_(N - 1)
        {
            const char *slash = strrchr(data_, '/');
            if(slash != nullptr)
            {
                data_ = slash + 1;
            }
            len_ = strlen(data_);
        }

        SourceFile(const char *name)
          : data_(name)
        {
            const char *slash = strrchr(data_, '/');
            if(slash != nullptr)
            {
                data_ = slash + 1;
            }
            len_ = strlen(data_);
        }

        const char *data_;
        size_t len_;
    };

    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, bool isSysErr);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    void formatLogHead();
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
        return flush_function;
    }
    
    SourceFile sourceFile_;
    int fileLine_;
    LogLevel level_;
    LogStream logStream_;
    Date date_;
};

#ifdef NDEBUG
#define LOG_TRACE                                                          \
    if (0)                                                                 \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kTrace, __func__) \
        .stream()
#else
#define LOG_TRACE                                                          \
    if (metro::Logger::logLevel() <= metro::Logger::kTrace)            \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kTrace, __func__) \
        .stream()
#endif
#define LOG_DEBUG                                                          \
    if (metro::Logger::logLevel() <= metro::Logger::kDebug)            \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kDebug, __func__) \
        .stream()
#define LOG_INFO                                               \
    if (metro::Logger::logLevel() <= metro::Logger::kInfo) \
    metro::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kWarn).stream()
#define LOG_ERROR \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kError).stream()
#define LOG_FATAL \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kFatal).stream()
#define LOG_SYSERR metro::Logger(__FILE__, __LINE__, true).stream()

#define LOG_TRACE_IF(cond)                                                  \
    if ((metro::Logger::logLevel() <= metro::Logger::kTrace) && (cond)) \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kTrace, __func__)  \
        .stream()
#define LOG_DEBUG_IF(cond)                                                \
    if ((metro::Logger::logLevel() <= metro::Logger::kDebug) && (cond)) \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kDebug, __func__)  \
        .stream()
#define LOG_INFO_IF(cond)                                                \
    if ((metro::Logger::logLevel() <= metro::Logger::kInfo) && (cond)) \
    metro::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN_IF(cond) \
    if (cond)             \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kWarn).stream()
#define LOG_ERROR_IF(cond) \
    if (cond)              \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kError).stream()
#define LOG_FATAL_IF(cond) \
    if (cond)              \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kFatal).stream()

#ifdef NDEBUG
#define DLOG_TRACE                                                         \
    if (0)                                                                 \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kTrace, __func__) \
        .stream()
#define DLOG_DEBUG                                                       \
    if (0)                                                               \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kDebug, __func__) \
        .stream()
#define DLOG_INFO \
    if (0)        \
    metro::Logger(__FILE__, __LINE__).stream()
#define DLOG_WARN \
    if (0)        \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kWarn).stream()
#define DLOG_ERROR \
    if (0)         \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kError).stream()
#define DLOG_FATAL \
    if (0)         \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kFatal).stream()

#define DLOG_TRACE_IF(cond)                                                \
    if (0)                                                                 \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kTrace, __func__) \
        .stream()
#define DLOG_DEBUG_IF(cond)                                              \
    if (0)                                                               \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kDebug, __func__) \
        .stream()
#define DLOG_INFO_IF(cond) \
    if (0)                 \
    metro::Logger(__FILE__, __LINE__).stream()
#define DLOG_WARN_IF(cond) \
    if (0)                 \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kWarn).stream()
#define DLOG_ERROR_IF(cond) \
    if (0)                  \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kError).stream()
#define DLOG_FATAL_IF(cond) \
    if (0)                  \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kFatal).stream()
#else
#define DLOG_TRACE                                                         \
    if (metro::Logger::logLevel() <= metro::Logger::kTrace)            \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kTrace, __func__) \
        .stream()
#define DLOG_DEBUG                                                       \
    if (metro::Logger::logLevel() <= metro::Logger::kDebug)            \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kDebug, __func__) \
        .stream()
#define DLOG_INFO                                            \
    if (metro::Logger::logLevel() <= metro::Logger::kInfo) \
    metro::Logger(__FILE__, __LINE__).stream()
#define DLOG_WARN \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kWarn).stream()
#define DLOG_ERROR \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kError).stream()
#define DLOG_FATAL \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kFatal).stream()

#define DLOG_TRACE_IF(cond)                                                 \
    if ((metro::Logger::logLevel() <= metro::Logger::kTrace) && (cond)) \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kTrace, __func__)  \
        .stream()
#define DLOG_DEBUG_IF(cond)                                               \
    if ((metro::Logger::logLevel() <= metro::Logger::kDebug) && (cond)) \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kDebug, __func__)  \
        .stream()
#define DLOG_INFO_IF(cond)                                               \
    if ((metro::Logger::logLevel() <= metro::Logger::kInfo) && (cond)) \
    metro::Logger(__FILE__, __LINE__).stream()
#define DLOG_WARN_IF(cond) \
    if (cond)              \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kWarn).stream()
#define DLOG_ERROR_IF(cond) \
    if (cond)               \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kError).stream()
#define DLOG_FATAL_IF(cond) \
    if (cond)               \
    metro::Logger(__FILE__, __LINE__, metro::Logger::kFatal).stream()
#endif


}
