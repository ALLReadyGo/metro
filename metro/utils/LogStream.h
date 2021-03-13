#pragma once

#include <string>
#include <array>
#include <algorithm>
#include "metro/utils/NonCopyable.h"
#include <cstring>

namespace metro
{

namespace detail
{

static constexpr size_t kSmallBufferSize = 4000;
static constexpr size_t kLargeBufferSize = 4000 * 1000;

template<size_t N>
class FixedBuffer
{
  public:
    FixedBuffer()
    {
        cur_ = data_.begin();
    }
    
    ~FixedBuffer()
    {
    }

    size_t length() const
    {
        return cur_ - data_.begin();
    }

    size_t size() const
    {
        return N;
    }

    size_t avalid() const 
    {
        return size() - length();
    }

    void reset()
    {
        cur_ = data_.begin();
    }

    void zeroBuffer()
    {
        data_.fill(0);
    }

    const char *data() const
    {
        return data_.begin();
    }

    std::string toString() const
    {
        return std::string(data_.begin(), length());
    }

    bool append(const char *buff, size_t len)
    {
        if(avalid() < len)
        {
            return false;
        }
        std::copy(buff, buff + len, cur_);
        cur_ += len;
        return true;
    }

  private:
    std::array<char, N> data_;
    char *cur_;
};

} // namespace detail


class LogStream : public NonCopyable
{
  public:
    
    LogStream &operator<<(bool v)
    {
        append(v ? "1" : "0", 1);
        return *this;
    }

    LogStream &operator<<(short);
    LogStream &operator<<(unsigned short);
    LogStream &operator<<(int);
    LogStream &operator<<(unsigned int);
    LogStream &operator<<(long);
    LogStream &operator<<(unsigned long);
    LogStream &operator<<(const long long &);
    LogStream &operator<<(const unsigned long long &);

    // LogStream &operator<<(const void *);

    LogStream &operator<<(float &v)
    {
        *this << static_cast<double>(v);
        return *this;
    }
    LogStream &operator<<(const double &);
    LogStream &operator<<(const long double &v);

    LogStream &operator<<(char v)
    {
        append(&v, 1);
        return *this;
    }

    // LogStream& operator<<(signed char);
    // LogStream& operator<<(unsigned char);
    template <int N>
    LogStream &operator<<(const char (&buf)[N])
    {
        assert(strnlen(buf, N) == N - 1);
        append(buf, N - 1);
        return *this;
    }

    LogStream &operator<<(char *str)
    {
        if (str)
        {
            append(str, strlen(str));
        }
        else
        {
            append("(null)", 6);
        }
        return *this;
    }

    LogStream &operator<<(const char *str)
    {
        if (str)
        {
            append(str, strlen(str));
        }
        else
        {
            append("(null)", 6);
        }
        return *this;
    }

    LogStream &operator<<(const unsigned char *str)
    {
        return operator<<(reinterpret_cast<const char *>(str));
    }

    LogStream &operator<<(const std::string &v)
    {
        append(v.c_str(), v.size());
        return *this;
    }

    void append(const char *buff, size_t len)
    {   
        if(exBuffer_.empty()) 
        {
            if(!fixBuffer_.append(buff, len))
            {
                exBuffer_.append(fixBuffer_.data(), fixBuffer_.length());
                exBuffer_.append(buff, len);
            }
        }
        else
        {
            exBuffer_.append(fixBuffer_.data(), fixBuffer_.length());
        }
    };

    const char *bufferData()
    {
        if(!exBuffer_.empty()) 
        {
            return exBuffer_.c_str();
        }
        return fixBuffer_.data();
    };

    size_t bufferLength()
    {
        if(!exBuffer_.empty())
        {
            return exBuffer_.length();
        }
        return fixBuffer_.length();
    };

    void reset()
    {
        fixBuffer_.reset();
        exBuffer_ = "";
    };
    
  private:

    template<typename T>
    void formatInt(T);

    detail::FixedBuffer<detail::kSmallBufferSize> fixBuffer_;
    std::string exBuffer_;
};



} // namespace metro