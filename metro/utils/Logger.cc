#include "metro/utils/Logger.h"
#include "assert.h"
#include <sys/syscall.h>
#include <unistd.h>

namespace metro
{

static thread_local pid_t threadId_ = 0;
static thread_local uint64_t lastSecond_ = 0;
static thread_local char lastTimeString_[32] = {0};

struct assertFixSize
{
    assertFixSize(const char *str, int len)
      : str_(str), len_(len)
    {
        assert(strlen(str_) == len_);
    };

    const char *str_;
    int len_;
};

LogStream &operator<<(LogStream &ls, const assertFixSize &t)
{
    ls.append(t.str_, t.len_);
    return ls;
}

LogStream &operator<<(LogStream &ls, const Logger::SourceFile &sf)
{
    ls.append(sf.data_, sf.len_);
    return ls;
}

void Logger::formatLogHead()
{
    uint64_t date_round_second = date_.roundSecond().microSecondsSinceEpoch();
    if(lastSecond_ != date_round_second)
    {
        lastSecond_ = date_round_second;
        strncpy(lastTimeString_, 
                date_.toFormattedStringLocal(false).c_str(), 
                sizeof(lastTimeString_));
    }
    logStream_ << assertFixSize(lastTimeString_, 17);
    char tmp[32];
    snprintf(tmp,
             sizeof(tmp), 
             ".%06lu UTC ", 
             (date_.microSecondsSinceEpoch() - date_round_second));
    logStream_ << assertFixSize(tmp, 12);
    if(threadId_ == 0)
    {
        threadId_ = static_cast<pid_t>(::syscall(SYS_gettid));
    }
    logStream_ << threadId_;
}

static std::array<const char*, Logger::LogLevel::kNumberOfLogLevels> logLevelStr =
{
    " TRACE ",
    " DEBUG ",
    " INFO  ",
    " WARN  ",
    " ERROR ",
    " FATAL "
};

const char *strerror_tl(int savedErrno)
{
    return strerror(savedErrno);
}


Logger::Logger(SourceFile file, int line)
  : sourceFile_(file), fileLine_(line),
    level_(kInfo), date_(Date::now())
{
    formatLogHead();
    logStream_ << assertFixSize(logLevelStr[level_], 7);
}

Logger::Logger(Logger::SourceFile file, int line, LogLevel level)
  : sourceFile_(file), fileLine_(line),
    level_(level), date_(Date::now())
{
    formatLogHead();
    logStream_ << assertFixSize(logLevelStr[level_], 7);
}

Logger::Logger(SourceFile file, int line, bool isSysErr)
  : sourceFile_(file), fileLine_(line), 
    level_(kFatal), date_(Date::now())
{
    formatLogHead();
    logStream_ << assertFixSize(logLevelStr[level_], 7);
    if (errno != 0)
    {
        logStream_ << strerror_tl(errno) << " (errno=" << errno << ") ";
    }
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : sourceFile_(file), fileLine_(line), date_(Date::now())
{
    formatLogHead();
    logStream_ << assertFixSize(logLevelStr[level_], 7) << "[" << func << "]";
}

Logger::~Logger()
{
    logStream_ << assertFixSize(" - ", 3) << sourceFile_ << ":" << fileLine_ << "\n";
    Logger::outputFunc_()(logStream_.bufferData(), logStream_.bufferLength());
    if(level_ > kError)
        flushFunc_();
}

LogStream &Logger::stream()
{
    return logStream_;
}

}