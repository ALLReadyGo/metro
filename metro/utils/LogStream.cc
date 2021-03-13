#include <numeric>
#include "metro/utils/LogStream.h"
#include <array>
#include <algorithm>

namespace metro
{

template<typename T>
int convert(char *buff, T v)
{
    int digit_num{0};
    T num{v};
    do
    {   
        int cur_digit = num % 10;
        buff[digit_num] = cur_digit + '0';
        digit_num += 1;
        num /= 10;
        
    } while(num != 0);
    if(v < 0)
    {
        buff[digit_num] = '-';
        digit_num += 1;
    }
    std::reverse(buff, buff + digit_num);
    return digit_num;
}

template<typename T>
void LogStream::formatInt(T v)
{
    constexpr size_t kReservieSize = std::numeric_limits<T>::digits10 + 4;
    std::array<char, kReservieSize> buff;
    size_t len = convert(buff.begin(), v);
    append(buff.begin(), len);
}

LogStream &LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned short v) 
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream &LogStream::operator<<(int v)
{
    formatInt(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned int v)
{
    formatInt(v);
    return *this;
}

LogStream &LogStream::operator<<(long v)
{
    formatInt(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long v)
{
    formatInt(v);
    return *this;
}

LogStream &LogStream::operator<<(const long long &v)
{
    formatInt(v);
    return *this;
}

LogStream &LogStream::operator<<(const unsigned long long &v)
{
    formatInt(v);
    return *this;
}

LogStream &LogStream::operator<<(const double &v)
{
    constexpr static int kReservieSize = 32;
    std::array<char, kReservieSize> buff;
    int len = snprintf(buff.begin(), kReservieSize, "%.12g", v);
    append(buff.begin(), len);
    return *this;
}

LogStream &LogStream::operator<<(const long double &v)
{
    constexpr static int kReservieSize = 32;
    std::array<char, kReservieSize> buff;
    int len = snprintf(buff.begin(), kReservieSize, "%.12Lg", v);
    append(buff.begin(), len);
    return *this;
}



} // namespace metro