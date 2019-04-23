//
//  Buffer.cpp
//  SignalServer
//
//  Created by raymon_wang on 2017/6/5.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#include "socket_buffer.hpp"

#include <sys/uio.h>

#include <webrtc/base/byteorder.h>

#include "rtc_log_wrapper.hpp"

using namespace rtc;

namespace reechat {
    const char SocketBuffer::kCRLF[] = "\r\n";
    
    const size_t SocketBuffer::kCheapPrependSize = 8;
    const size_t SocketBuffer::kInitialSize  = 65536;
    
    SocketBuffer::SocketBuffer(size_t initial_size, size_t reserved_prepend_size) :
    capacity_(reserved_prepend_size + initial_size),
    read_index_(reserved_prepend_size),
    write_index_(reserved_prepend_size),
    reserved_prepend_size_(reserved_prepend_size)
    {
        buffer_ = new char[capacity_];
        assert(length() == 0);
        assert(WritableBytes() == initial_size);
        assert(PrependableBytes() == reserved_prepend_size);
    }
    
    SocketBuffer::~SocketBuffer()
    {
        delete[] buffer_;
        buffer_ = nullptr;
        capacity_ = 0;
    }
    
    void SocketBuffer::Skip(size_t len)
    {
        if (len < length()) {
            read_index_ += len;
        } else {
            Reset();
        }
    }
    
    void SocketBuffer::Retrieve(size_t len)
    {
        Skip(len);
    }
    
    void SocketBuffer::Truncate(size_t n)
    {
        if (n == 0) {
            read_index_ = reserved_prepend_size_;
            write_index_ = reserved_prepend_size_;
        } else if (write_index_ > read_index_ + n) {
            write_index_ = read_index_ + n;
        }
    }
    
    void SocketBuffer::Reset()
    {
        Truncate(0);
    }
    
    void SocketBuffer::Reserve(size_t len)
    {
        if (capacity_ >= len + reserved_prepend_size_) {
            return;
        }
        
        // TODO add the implementation logic here
        grow(len + reserved_prepend_size_);
    }
    
    void SocketBuffer::EnsureWritableBytes(size_t len)
    {
        if (WritableBytes() < len) {
            grow(len);
        }
        
        assert(WritableBytes() >= len);
    }
    
    void SocketBuffer::ToText()
    {
        AppendInt8('\0');
        UnwriteBytes(1);
    }
    
    void SocketBuffer::Write(const void* /*restrict*/ d, size_t len)
    {
        EnsureWritableBytes(len);
        memcpy(WriteBegin(), d, len);
        assert(write_index_ + len <= capacity_);
        write_index_ += len;
    }
    
    void SocketBuffer::Append(const char* /*restrict*/ d, size_t len)
    {
        Write(d, len);
    }
    
    void SocketBuffer::Append(const void* /*restrict*/ d, size_t len)
    {
        Write(d, len);
    }
    
    void SocketBuffer::AppendInt64(int64_t x)
    {
        int64_t be = evppbswap_64(x);
        Write(&be, sizeof be);
    }
    
    void SocketBuffer::AppendInt32(int32_t x)
    {
        int32_t be32 = htonl(x);
        Write(&be32, sizeof be32);
    }
    
    void SocketBuffer::AppendInt16(int16_t x)
    {
        int16_t be16 = htons(x);
        Write(&be16, sizeof be16);
    }
    
    void SocketBuffer::AppendInt8(int8_t x)
    {
        Write(&x, sizeof x);
    }
    
    void SocketBuffer::PrependInt64(int64_t x)
    {
        int64_t be = evppbswap_64(x);
        Prepend(&be, sizeof be);
    }
    
    void SocketBuffer::PrependInt32(int32_t x)
    {
        int32_t be32 = htonl(x);
        Prepend(&be32, sizeof be32);
    }
    
    void SocketBuffer::PrependInt16(int16_t x)
    {
        int16_t be16 = htons(x);
        Prepend(&be16, sizeof be16);
    }
    
    void SocketBuffer::PrependInt8(int8_t x)
    {
        Prepend(&x, sizeof x);
    }
    
    // Insert content, specified by the parameter, into the front of reading index
    void SocketBuffer::Prepend(const void* /*restrict*/ d, size_t len)
    {
        assert(len <= PrependableBytes());
        read_index_ -= len;
        const char* p = static_cast<const char*>(d);
        memcpy(begin() + read_index_, p, len);
    }
    
    void SocketBuffer::UnwriteBytes(size_t n)
    {
        assert(n <= length());
        write_index_ -= n;
    }
    
    void SocketBuffer::WriteBytes(size_t n)
    {
        assert(n <= WritableBytes());
        write_index_ += n;
    }
    
    int64_t SocketBuffer::ReadInt64()
    {
        int64_t result = PeekInt64();
        Skip(sizeof result);
        return result;
    }
    
    int32_t SocketBuffer::ReadInt32()
    {
        int32_t result = PeekInt32();
        Skip(sizeof result);
        return result;
    }
    
    int16_t SocketBuffer::ReadInt16()
    {
        int16_t result = PeekInt16();
        Skip(sizeof result);
        return result;
    }
    
    uint16_t SocketBuffer::ReadUInt16()
    {
        int16_t result = PeekInt16();
        Skip(sizeof result);
        return result;
    }
    
    int8_t SocketBuffer::ReadInt8()
    {
        int8_t result = PeekInt8();
        Skip(sizeof result);
        return result;
    }
    
    std::string SocketBuffer::ToString() const
    {
        return std::string(data(), length());
    }
    
    ssize_t SocketBuffer::ReadFromFD(int fd, int* savedErrno)
    {
        // saved an ioctl()/FIONREAD call to tell how much to read
        char extrabuf[65536];
        struct iovec vec[2];
        const size_t writable = WritableBytes();
        vec[0].iov_base = begin() + write_index_;
        vec[0].iov_len = writable;
        vec[1].iov_base = extrabuf;
        vec[1].iov_len = sizeof extrabuf;
        // when there is enough space in this buffer, don't read into extrabuf.
        // when extrabuf is used, we read 64k bytes at most.
        const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
        const ssize_t n = ::readv(fd, vec, iovcnt);
        
        if (n < 0) {
            *savedErrno = errno;
        } else if (static_cast<size_t>(n) <= writable) {
            write_index_ += n;
        } else {
            write_index_ = capacity_;
            Append(extrabuf, n - writable);
        }
        
        return n;
    }
    
    void SocketBuffer::Next(size_t len)
    {
        if (len <= length())
            read_index_ += len;
    }
    
    char SocketBuffer::ReadByte()
    {
        assert(length() >= 1);
        if (length() == 0) {
            return '\0';
        }
        return buffer_[read_index_++];
    }
    
    void SocketBuffer::UnreadBytes(size_t n)
    {
        assert(n < read_index_);
        read_index_ -= n;
    }
    
    uint16_t SocketBuffer::PeekUInt16() const
    {
        assert(length() >= sizeof(uint16_t));
        uint16_t be16 = 0;
        ::memcpy(&be16, data(), sizeof be16);
        return NetworkToHost16(be16);
    }
    
    int8_t SocketBuffer::PeekInt8() const
    {
        assert(length() >= sizeof(int8_t));
        int8_t x = *data();
        return x;
    }
    
    void SocketBuffer::grow(size_t len)
    {
        if (WritableBytes() + PrependableBytes() < len + reserved_prepend_size_) {
            //grow the capacity
            size_t n = (capacity_ << 1) + len;
            size_t m = length();
            char* d = new char[n];
            memcpy(d + reserved_prepend_size_, begin() + read_index_, m);
            write_index_ = m + reserved_prepend_size_;
            read_index_ = reserved_prepend_size_;
            capacity_ = n;
            delete[] buffer_;
            buffer_ = d;
        } else {
            // move readable data to the front, make space inside buffer
            assert(reserved_prepend_size_ < read_index_);
            size_t readable = length();
            memmove(begin() + reserved_prepend_size_, begin() + read_index_, length());
            read_index_ = reserved_prepend_size_;
            write_index_ = read_index_ + readable;
            assert(readable == length());
            assert(WritableBytes() >= len);
        }
    }
    
    RingBuffer::RingBuffer(uint32_t capacity) :
    capacity_(capacity),
    max_index_(capacity - 1),
    read_count_(0),
    write_count_(0)
    {
        data_arr_ = new char[capacity];
    }
    
    RingBuffer::~RingBuffer()
    {
        SAFE_DELETEARRAY(data_arr_);
    }
    
    bool RingBuffer::IsEmpty()
    {
        return read_count_ == write_count_;
    }
    
    const char* RingBuffer::DataHead()
    {
        return data_arr_;
    }
    
    uint32_t RingBuffer::AvailableRead()
    {
        uint32_t available_read = write_count_ - read_count_;
        assert(available_read < capacity_);
        return available_read;
    }
    
    uint32_t RingBuffer::AvailableSeriesRead()
    {
        uint32_t read_count = read_count_;
        uint32_t available_read = write_count_ - read_count;
        assert(capacity_ >= available_read);
        if (available_read == 0) {
            return 0;
        }
        
        uint32_t read_index = read_count & max_index_;
        if (read_index + available_read <= capacity_) {
            return available_read;
        }
        else {
            return capacity_ - read_index;
        }
    }
    
    const char* RingBuffer::CurrentSeriesReadHead()
    {
        uint32_t read_index = read_count_ & max_index_;
        return data_arr_ + read_index;
    }
    
    void RingBuffer::ConsumeSeriesReadLen(uint32_t len)
    {
        uint32_t read_index = read_count_ & max_index_;
        assert(read_index + len <= capacity_);
        read_count_ += len;
    }
    
    bool RingBuffer::WriteContent(const char* content, uint32_t len)
    {
        //计算可写数量
        uint32_t write_count = write_count_;
        uint32_t content_size = write_count - read_count_;
        assert(capacity_ >= content_size);
        uint32_t available_size = capacity_ - content_size;
        //如果可写数量小于len,缓存满，返回错误信息
        if (available_size < len) {
            RTCHAT_LOG(LS_VERBOSE) << "ringbuffer have not enough space to store new data";
            return false;
        }
        
        uint32_t write_index = write_count & max_index_;
        if (write_index + len <= capacity_) {
            memcpy((data_arr_ + write_index), content, len);
        }
        else {
            uint32_t tail_left = capacity_ - write_index;
            memcpy(data_arr_ + write_index, content, tail_left);
            memcpy(data_arr_, content + tail_left, len - tail_left);
        }
        write_count_ += len;
        
        return true;
    }
    
    uint32_t RingBuffer::AvailableWrite()
    {
        uint32_t content_size = write_count_ - read_count_;
        assert(capacity_ >= content_size);
        return capacity_ - content_size;
    }
    
    char* RingBuffer::CurrentSeriesWriteHead()
    {
        uint32_t write_index = write_count_ & max_index_;
        return data_arr_ + write_index;
    }
    
    uint32_t RingBuffer::AvailableSeriesWrite()
    {
        uint32_t read_count = read_count_;
        uint32_t write_count = write_count_;
        uint32_t content_size = write_count - read_count;
        assert(capacity_ >= content_size);
        uint32_t available_write = capacity_ - content_size;;
        uint32_t write_index = write_count & max_index_;
        
        if (write_index + available_write > capacity_) {
            return capacity_ - write_index;
        }
        else {
            return available_write;
        }
    }
    
    void RingBuffer::ConsumeSeriesWriteLen(uint32_t len)
    {
        uint32_t write_index = write_count_ & max_index_;
        assert(write_index + len <= capacity_);
        write_count_ += len;
    }
}







