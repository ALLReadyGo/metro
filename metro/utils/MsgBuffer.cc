#include "metro/utils/MsgBuffer.h"
#include <netinet/in.h>
#include <sys/uio.h>

namespace metro
{

static constexpr size_t kBufferOffset = 8;

inline uint64_t hton64(uint64_t n)
{
    static const int one = 1;
    static const char sig = *(char *)&one;
    if (sig == 0)
        return n; 
    char *ptr = reinterpret_cast<char *>(&n);
    std::reverse(ptr, ptr + sizeof(uint64_t));
    return n;
}

inline uint64_t ntoh64(uint64_t n)
{
    return hton64(n);
}

MsgBuffer::MsgBuffer(size_t len)
  : head_(kBufferOffset),
    initCap_(len),
    buffer_(len + kBufferOffset),
    tail_(kBufferOffset)
{
}

void MsgBuffer::ensureWritableBytes(size_t len)
{
    if(writableBytes() >= len)
        return;
    if(head_ + writableBytes() >= (len + kBufferOffset))
    {
        std::copy(begin() + head_, begin() + tail_, begin() + kBufferOffset);
        tail_ = kBufferOffset + (tail_ - head_);
        head_ = kBufferOffset;
        return;
    }
    size_t new_len;
    if((buffer_.size() * 2) > kBufferOffset + readableBytes() + len)
        new_len = buffer_.size() * 2;
    else
        new_len = kBufferOffset + readableBytes() + len;
    MsgBuffer new_buffer(new_len);
    new_buffer.append(*this);
    swap(new_buffer);
}

uint16_t MsgBuffer::peekInt16() const
{
    assert(readableBytes() >= 2);
    uint16_t rs = *(static_cast<uint16_t *>((void *)peek()));
    return ntohs(rs);
}

uint32_t MsgBuffer::peekInt32() const
{
    assert(readableBytes() >= 4);
    uint32_t rs = *(static_cast<uint32_t *>((void *)peek()));
    return ntohl(rs);
}   

uint64_t MsgBuffer::peekInt64() const
{
    assert(readableBytes() >= 8);
    uint64_t rs = *(static_cast<uint64_t *>((void *)peek()));
    return ntoh64(rs);
}

/* read */
std::string MsgBuffer::read(size_t len)
{
    if(len > readableBytes())
        len = readableBytes();
    std::string ret(peek(), len);
    retrieve(len);
    return ret;
}

uint8_t MsgBuffer::readInt8()
{
    uint8_t ret = peekInt8();
    retrieve(1);
    return ret;
}

uint16_t MsgBuffer::readInt16()
{
    uint16_t ret = peekInt16();
    retrieve(2);
    return ret;
}

uint32_t MsgBuffer::readInt32()
{
    uint32_t ret = peekInt32();
    retrieve(4);
    return ret;
}

uint64_t MsgBuffer::readInt64()
{
    uint64_t ret = peekInt64();
    retrieve(8);
    return ret;
}

    /* append */
void MsgBuffer::append(const MsgBuffer &buf)
{
    ensureWritableBytes(buf.readableBytes());
    std::copy(buf.peek(), buf.peek() + tail_, beginWrite());
    tail_ += buf.readableBytes();
}

void MsgBuffer::append(const char *buf, size_t len)
{
    ensureWritableBytes(len);
    std::copy(buf, buf + len, beginWrite());
    tail_ += len;
}

void MsgBuffer::appendInt16(const uint16_t s)
{
    uint16_t in = htons(s);
    append(static_cast<char *>((void *)&in), sizeof(s));
}

void MsgBuffer::appendInt32(const uint32_t i)
{
    uint32_t in = htonl(i);
    append(static_cast<char *>((void *)&in), sizeof(i));
}

void MsgBuffer::appendInt64(const uint64_t l)
{
    uint64_t in = hton64(l);
    append(static_cast<char *>((void *)&in), sizeof(l));
}

/* addInFront */
void MsgBuffer::addInFront(const char *buf, size_t len)
{
    if(head_ >= len)
    {
        std::copy(buf, buf + len, const_cast<char *>(peek()) - len);
        head_ -= len;
        return;
    }
    if(len <= writableBytes())
    {
        std::copy(begin() + head_, begin() + tail_, begin() + head_ + len);
        std::copy(buf, buf + len, begin() + head_);
        tail_ += len;
        return;
    }
    size_t newLen;
    if(len + readableBytes() < initCap_)
        newLen = initCap_;
    else
        newLen = len + readableBytes();
    MsgBuffer newBuf(newLen);
    newBuf.append(buf, len);
    newBuf.append(*this);
    swap(newBuf);                               
}
    
void MsgBuffer::addInFroundInt16(const uint16_t s)
{
    uint16_t in = htons(s);
    addInFront(static_cast<char *>((void *)&in), sizeof(in));
}

void MsgBuffer::addInFroundInt32(const uint32_t i)
{
    uint32_t in = htons(i);
    addInFront(static_cast<char *>((void *)&in), sizeof(in));
}

void MsgBuffer::addInFroundInt64(const uint64_t l)
{
    uint64_t in = htons(l);
    addInFront(static_cast<char *>((void *)&in), sizeof(in));
}


void MsgBuffer::retrieveAll()
{
    if(buffer_.size() > (initCap_ * 2))
    {
        buffer_.resize(initCap_);
    }
    tail_ = head_ = kBufferOffset;                 // 如果数据全部取出完毕，head_不要忘记重置回头部
}

void MsgBuffer::retrieve(size_t len)
{
    if(len > readableBytes())
    {
        retrieveAll();
        return;
    }
    head_ += len;
}

/* 从fd中读取数据到buffer当中，socket中的数据获取就是通过这个API进行的 */
ssize_t MsgBuffer::readFd(int fd, int *retError)
{
    char exBuffer[8192];
    struct iovec vec[2];                        // 使用两个缓冲区进行数据读取，保证能够及时从socket中取出数据，大吞吐量数据传输时很有必要
    size_t writable = writableBytes();
    vec[0].iov_base = begin() + tail_;
    vec[0].iov_len = writable;
    vec[1].iov_base = exBuffer;
    vec[1].iov_len = sizeof(exBuffer);
    const int iovcnt = (writable < sizeof(exBuffer)) ? 2 : 1;
    ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0)
    {
        *retError = errno;
    }
    else if(n <= writable)                      // 不需要exBuffer
    {
        tail_ += n;
    }
    else
    {
        tail_ = buffer_.size();                 // 此时exBuffer被使用
        append(exBuffer, n - writable);
    }
    return n;
}

void MsgBuffer::swap(MsgBuffer &other) noexcept
{
    buffer_.swap(other.buffer_);
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    std::swap(initCap_, other.initCap_);
}
}