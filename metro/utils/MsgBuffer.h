#pragma once

#include <metro/utils/NonCopyable.h>
#include <vector>
#include <string>
#include <assert.h>
#include <algorithm>

namespace metro
{

static constexpr size_t kBufferDafaultLength = 2048;
static constexpr char CRLF[] = "\r\n";


class MsgBuffer
{
  public:
    MsgBuffer(size_t len = kBufferDafaultLength);

    const char *peek() const
    {
        return begin() + head_;
    }

    const char *beginWrite() const
    {
        return begin() + tail_;
    }

    char *beginWrite()
    {
        return begin() + tail_;
    }

    uint8_t peekInt8() const
    {
        assert(readableBytes() >= 1);
        return *(static_cast<const uint8_t *>((void *)peek()));
    }

    uint16_t peekInt16() const;

    uint32_t peekInt32() const;

    uint64_t peekInt64() const;

    std::string read(size_t len);

    /* read */
    uint8_t readInt8();

    uint16_t readInt16();

    uint32_t readInt32();

    uint64_t readInt64();

    void swap(MsgBuffer &other) noexcept;

    size_t readableBytes() const
    {
        return tail_ - head_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - tail_;
    }

    /* append */
    void append(const MsgBuffer &buf);
    void append(const char *buf, size_t len);
    template <int N>
    void append(const char (&buf)[N])
    {
        append(buf, N - 1);
    }
    void append(const std::string& buf)
    {
        append(buf.c_str(), buf.size() - 1);        
    }

    void appendInt8(const uint8_t b)
    {
        append(static_cast<const char *>((void *)&b), 1);
    }

    void appendInt16(const uint16_t s);

    void appendInt32(const uint32_t i);

    void appendInt64(const uint64_t l);

    /* addInFront */
    void addInFront(const char *buf, size_t len);

    void addInFroundInt8(const uint8_t b)
    {
        addInFront(static_cast<const char *>((void *)&b), 1);
    }

    void addInFroundInt16(const uint16_t s);

    void addInFroundInt32(const uint32_t i);

    void addInFroundInt64(const uint64_t l);

    void retrieveAll();


    ssize_t readFd(int fd, int *retError);

    void retrieve(size_t len);

    void retrieveUntil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    const char *findCRLF() const
    {
        const char *crlf = std::search(peek(), beginWrite(), CRLF, CRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    void ensureWritableBytes(size_t len);

    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        tail_ += len;
    }

    void unwrite(size_t offset)
    {
        assert(readableBytes() >= offset);
        tail_ -= offset;
    }

    const char &operator[](size_t offset) const
    {
        assert(readableBytes() > offset);
        return peek()[offset];
    }

    char &operator[](size_t offset)
    {
        assert(readableBytes() >= offset);
        return begin()[head_ + offset];
    }

  private:

    size_t head_;
    size_t initCap_;
    std::vector<char> buffer_;
    size_t tail_;
    const char *begin() const
    {
        return &buffer_[0];
    }

    char *begin()
    {
        return &buffer_[0];
    }
};

inline void swap(MsgBuffer &one, MsgBuffer &two) noexcept
{
    one.swap(two);
}

}