#pragma once

#include <ctime>
#include <string>

namespace metro
{

constexpr int kMicroSecondPerSec = 1000000;

class Date
{
  public:
    Date()
      : microSecondsSinceEpoch_(0)
    {
    }

    explicit Date(uint64_t microSecond)
      : microSecondsSinceEpoch_(microSecond)
    {
    }

    Date(unsigned int year,
         unsigned int month,
         unsigned int day,
         unsigned int hour = 0,
         unsigned int minute = 0,
         unsigned int second = 0,
         unsigned int microSecond = 0);

    static const Date date();

    static const Date now()
    {
        return Date::now();
    }

    const Date after(double second) const;
    
    const Date roundSecond() const;

    const Date roundDay() const;

    ~Date()
    {
    }

    bool operator==(const Date &other) const
    {
        return microSecondsSinceEpoch_ == other.microSecondsSinceEpoch_;
    }

    bool operator!=(const Date &other) const
    {
        return microSecondsSinceEpoch_ != other.microSecondsSinceEpoch_;
    }

    bool operator<(const Date &other) const
    {
        return microSecondsSinceEpoch_ < other.microSecondsSinceEpoch_;
    }

    bool operator>(const Date &other) const
    {
        return microSecondsSinceEpoch_ > other.microSecondsSinceEpoch_;
    }

    
    bool operator<=(const Date &other) const
    {
        return microSecondsSinceEpoch_ <= other.microSecondsSinceEpoch_;
    }

    bool operator>=(const Date &other) const
    {
        return microSecondsSinceEpoch_ >= other.microSecondsSinceEpoch_;
    }

    int64_t microSecondsSinceEpoch() const
    {
        return microSecondsSinceEpoch_;
    }

    int64_t secondsSinceEpoch() const
    {
        return microSecondsSinceEpoch_ / kMicroSecondPerSec;
    }

    struct tm tmStruct() const;

    /**
     * @brief 显式UTC时间字符串
     * @example
     * 20210101 21:14:15            // showMicroSeconds == false
     * 20210101 21:14:15:2000       // showMicroSeconds == true
     */ 
    std::string toFormattedString(bool showMicroSeconds) const;

    
    std::string toCustomedFormattedString(const std::string &fmtStr, 
                                          bool showMicroSeconds = false) const;
    
    std::string toFormattedStringLocal(bool showMicroSeconds) const;

    std::string toCustomedFormattedStringLocal(const std::string &fmtStr, 
                                               bool showMicroSeconds = false) const;
    
    std::string toDbStringLocal() const;

    static Date fromDbStringLocal(const std::string &dbtime);

    void toCustomedFormattedString(const std::string &fmtStr, 
                                   char *str,
                                   size_t len) const;
    
    bool isSameSecond(const Date &other) const
    {
        return microSecondsSinceEpoch_ / kMicroSecondPerSec == 
               other.microSecondsSinceEpoch_ / kMicroSecondPerSec;
    }

    void swap(Date &other)
    {
        std::swap(microSecondsSinceEpoch_, other.microSecondsSinceEpoch_);
    }

  private:
    uint64_t microSecondsSinceEpoch_; 
};


}

