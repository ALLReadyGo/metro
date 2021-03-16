#include "metro/utils/Date.h"
#include <sys/time.h>
#include <vector>
#include "metro/utils/Funcs.h"
namespace metro
{

Date::Date(unsigned int year,
           unsigned int month,
           unsigned int day,
           unsigned int hour,
           unsigned int minute,
           unsigned int second,
           unsigned int microSecond) 
    {
        // TODO:s
    }
    
const Date Date::date() 
{   
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    Date date(tv.tv_sec * kMicroSecondPerSec + tv.tv_usec);
    return date;
}

const Date Date::after(double second) const
{
    Date date(microSecondsSinceEpoch_ + 
              static_cast<uint64_t>(second * kMicroSecondPerSec));
    return date;
}

const Date Date::roundSecond() const
{
    uint64_t seconds = microSecondsSinceEpoch_ / kMicroSecondPerSec;
    return Date(seconds * kMicroSecondPerSec);
}
    
const Date Date::roundDay() const
{
    constexpr uint64_t kMicroSecondPerDay = kMicroSecondPerSec * 60 * 60 * 24;
    uint64_t days = microSecondsSinceEpoch_ / kMicroSecondPerDay;
    return Date(days * kMicroSecondPerDay);
}

struct tm Date::tmStruct() const
{
    time_t seconds = 
        static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSec);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    return tm_time;
}

std::string Date::toFormattedString(bool showMicroSeconds) const
{
    char buff[128] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSec);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    if(showMicroSeconds)
    {
        int micsecond = microSecondsSinceEpoch_ % kMicroSecondPerSec;
        snprintf(buff, 
                 sizeof(buff), 
                 "%4d%02d%02d %02d:%02d:%02d:%06d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec,
                 micsecond 
                );
    }
    else
    {
        snprintf(buff, 
                 sizeof(buff), 
                 "%4d%02d%02d %02d:%02d:%02d", 
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec
                );
    }
    return buff;
}

std::string Date::toCustomedFormattedString(const std::string &fmtStr, 
                                            bool showMicroSeconds) const
{
    char buff[256] = {0};
    time_t second = 
        static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSec);
    struct tm tm_time;
    gmtime_r(&second, &tm_time);
    strftime(buff, sizeof(buff), fmtStr.c_str(), &tm_time);
    if(!showMicroSeconds)
        return buff;
    int microsecond = microSecondsSinceEpoch_ % kMicroSecondPerSec;
    char ms_buff[12] = {0};
    snprintf(ms_buff, sizeof(ms_buff), "%06d", microsecond);
    return std::string(buff) + std::string(ms_buff);
}

void Date::toCustomedFormattedString(const std::string &fmtStr, 
                                     char *str,
                                     size_t len) const
{
    time_t second = 
        static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSec);
    struct tm tm_time;
    gmtime_r(&second, &tm_time);
    strftime(str, len, fmtStr.c_str(), &tm_time);
}

std::string Date::toFormattedStringLocal(bool showMicroSeconds) const
{
    char buff[128] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSec);
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);

    if(showMicroSeconds)
    {
        int micsecond = microSecondsSinceEpoch_ % kMicroSecondPerSec;
        snprintf(buff, 
                 sizeof(buff), 
                 "%4d%02d%02d %02d:%02d:%02d:%06d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec,
                 micsecond 
                );
    }
    else
    {
        snprintf(buff, 
                 sizeof(buff), 
                 "%4d%02d%02d %02d:%02d:%02d", 
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec
                );
    }
    return buff;
}

std::string Date::toCustomedFormattedStringLocal(const std::string &fmtStr, 
                                                 bool showMicroSeconds = false) const
{
    char buff[256] = {0};
    time_t second = 
        static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSec);
    struct tm tm_time;
    localtime_r(&second, &tm_time);
    strftime(buff, sizeof(buff), fmtStr.c_str(), &tm_time);
    if(!showMicroSeconds)
        return buff;
    int microsecond = microSecondsSinceEpoch_ % kMicroSecondPerSec;
    char ms_buff[12] = {0};
    snprintf(ms_buff, sizeof(ms_buff), "%06d", microsecond);
    return std::string(buff) + std::string(ms_buff);
}
    
std::string Date::toDbStringLocal() const
{
    char buff[128] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSec);
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);
    bool show_micro_seconds = microSecondsSinceEpoch_ % kMicroSecondPerSec;
    if(show_micro_seconds)
    {
        int microseconds =
            static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondPerSec);
        snprintf(buff,
                 sizeof(buff),
                 "%4d-%02d-%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        if (*this == roundDay())
        {
            snprintf(buff,
                     sizeof(buff),
                     "%4d-%02d-%02d",
                     tm_time.tm_year + 1900,
                     tm_time.tm_mon + 1,
                     tm_time.tm_mday);
        }
        else
        {
            snprintf(buff,
                     sizeof(buff),
                     "%4d-%02d-%02d %02d:%02d:%02d",
                     tm_time.tm_year + 1900,
                     tm_time.tm_mon + 1,
                     tm_time.tm_mday,
                     tm_time.tm_hour,
                     tm_time.tm_min,
                     tm_time.tm_sec);
        }
    }
    return buff;
}

Date Date::fromDbStringLocal(const std::string &datetime)
{
    unsigned int year{0}, month{0}, day{0}, hour{0}, minute{0},
                 second{0}, microSecond{0};

    std::vector<std::string> v = splitString(datetime, " ");
    if (2 == v.size())
    {
        std::vector<std::string> date = splitString(v[0], "-");
        if (3 == date.size())
        {
            year = std::stol(date[0]);
            month = std::stol(date[1]);
            day = std::stol(date[2]);
            std::vector<std::string> time = splitString(v[1], ":");
            if (2 < time.size())
            {
                hour = std::stol(time[0]);
                minute = std::stol(time[1]);
                auto seconds = splitString(time[2], ".");
                second = std::stol(seconds[0]);
                if (1 < seconds.size())
                {
                    if (seconds[1].length() > 6)
                    {
                        seconds[1].resize(6);
                    }
                    else if (seconds[1].length() < 6)
                    {
                        seconds[1].append(6 - seconds[1].length(), '0');
                    }
                    microSecond = std::stol(seconds[1]);
                }
            }
        }
    }
    return Date(year, month, day, hour, minute, second, microSecond);
}

}