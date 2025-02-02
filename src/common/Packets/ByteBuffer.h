/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include "Common.h"
#include "Errors.h"
#include "Util.h"
#include "ByteConverter.h"
#include <exception>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <cstring>

// Root of ByteBuffer exception hierarchy
class ByteBufferException : public std::exception
{
public:
    ~ByteBufferException() noexcept override = default;

    [[nodiscard]] char const* what() const noexcept override { return msg_.c_str(); }

protected:
    std::string& message() noexcept { return msg_; }

private:
    std::string msg_;
};

class ByteBufferPositionException : public ByteBufferException
{
public:
    ByteBufferPositionException(bool add, size_t pos, size_t size, size_t valueSize);
    ~ByteBufferPositionException() noexcept override = default;
};

class ByteBufferSourceException : public ByteBufferException
{
public:
    ByteBufferSourceException(size_t pos, size_t size, size_t valueSize);
    ~ByteBufferSourceException() noexcept override = default;
};

class ByteBuffer
{
public:
    const static size_t DEFAULT_SIZE = 0x1000;

    // constructor
    ByteBuffer()  
    {
        _storage.reserve(DEFAULT_SIZE);
    }

    ByteBuffer(size_t reserve) : _rpos(0), _wpos(0)
    {
        _storage.reserve(reserve);
    }

    // copy constructor
    ByteBuffer(const ByteBuffer& buf) : _rpos(buf._rpos), _wpos(buf._wpos),
        _storage(buf._storage)
    {
    }
    /* requried as of C++ 11 */
#if __cplusplus >= 201103L
    ByteBuffer(ByteBuffer&&) = default;
    ByteBuffer& operator=(const ByteBuffer&) = default;
    ByteBuffer& operator=(ByteBuffer&&) = default;
#endif

    void clear()
    {
        _storage.clear();
        _rpos = _wpos = 0;
    }

    template <typename T> void append(T value)
    {
        EndianConvert(value);
        append((uint8*)&value, sizeof(value));
    }

    template <typename T> void put(size_t pos, T value)
    {
        EndianConvert(value);
        put(pos, (uint8*)&value, sizeof(value));
    }

    ByteBuffer& operator<<(uint8 value)
    {
        append<uint8>(value);
        return *this;
    }

    ByteBuffer& operator<<(uint16 value)
    {
        append<uint16>(value);
        return *this;
    }

    ByteBuffer& operator<<(uint32 value)
    {
        append<uint32>(value);
        return *this;
    }

    ByteBuffer& operator<<(uint64 value)
    {
        append<uint64>(value);
        return *this;
    }

    // signed as in 2e complement
    ByteBuffer& operator<<(int8 value)
    {
        append<int8>(value);
        return *this;
    }

    ByteBuffer& operator<<(int16 value)
    {
        append<int16>(value);
        return *this;
    }

    ByteBuffer& operator<<(int32 value)
    {
        append<int32>(value);
        return *this;
    }

    ByteBuffer& operator<<(int64 value)
    {
        append<int64>(value);
        return *this;
    }

    // floating points
    ByteBuffer& operator<<(float value)
    {
        append<float>(value);
        return *this;
    }

    ByteBuffer& operator<<(double value)
    {
        append<double>(value);
        return *this;
    }

    ByteBuffer& operator<<(const std::string& value)
    {
        if (size_t len = value.length())
            append((uint8 const*)value.c_str(), len);
        append((uint8)0);
        return *this;
    }

    ByteBuffer& operator<<(const char* str)
    {
        if (size_t len = (str ? strlen(str) : 0))
            append((uint8 const*)str, len);
        append((uint8)0);
        return *this;
    }

    ByteBuffer& operator>>(bool& value)
    {
        value = (read<char>() > 0);
        return *this;
    }

    ByteBuffer& operator>>(uint8& value)
    {
        value = read<uint8>();
        return *this;
    }

    ByteBuffer& operator>>(uint16& value)
    {
        value = read<uint16>();
        return *this;
    }

    ByteBuffer& operator>>(uint32& value)
    {
        value = read<uint32>();
        return *this;
    }

    ByteBuffer& operator>>(uint64& value)
    {
        value = read<uint64>();
        return *this;
    }

    //signed as in 2e complement
    ByteBuffer& operator>>(int8& value)
    {
        value = read<int8>();
        return *this;
    }

    ByteBuffer& operator>>(int16& value)
    {
        value = read<int16>();
        return *this;
    }

    ByteBuffer& operator>>(int32& value)
    {
        value = read<int32>();
        return *this;
    }

    ByteBuffer& operator>>(int64& value)
    {
        value = read<int64>();
        return *this;
    }

    ByteBuffer& operator>>(float& value)
    {
        value = read<float>();
        if (!myisfinite(value))
        {
            value = 0.0f;
            //throw ByteBufferException();
        }
        return *this;
    }

    ByteBuffer& operator>>(double& value)
    {
        value = read<double>();
        if (!myisfinite(value))
        {
            value = 0.0f;
            //throw ByteBufferException();
        }
        return *this;
    }

    ByteBuffer& operator>>(std::string& value)
    {
        value.clear();
        while (rpos() < size())                         // prevent crash at wrong string format in packet
        {
            char c = read<char>();
            if (c == 0)
                break;
            value += c;
        }
        return *this;
    }

    uint8& operator[](size_t const pos)
    {
        if (pos >= size())
            throw ByteBufferPositionException(false, pos, 1, size());
        return _storage[pos];
    }

    uint8 const& operator[](size_t const pos) const
    {
        if (pos >= size())
            throw ByteBufferPositionException(false, pos, 1, size());
        return _storage[pos];
    }

    [[nodiscard]] size_t rpos() const { return _rpos; }

    size_t rpos(size_t rpos_)
    {
        _rpos = rpos_;
        return _rpos;
    }

    void rfinish()
    {
        _rpos = wpos();
    }

    [[nodiscard]] size_t wpos() const { return _wpos; }

    size_t wpos(size_t wpos_)
    {
        _wpos = wpos_;
        return _wpos;
    }

    template<typename T>
    void read_skip() { read_skip(sizeof(T)); }

    void read_skip(size_t skip)
    {
        if (_rpos + skip > size())
            throw ByteBufferPositionException(false, _rpos, skip, size());
        _rpos += skip;
    }

    template <typename T> T read()
    {
        T r = read<T>(_rpos);
        _rpos += sizeof(T);
        return r;
    }

    template <typename T> [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] T read(size_t pos) const
    {
        if (pos + sizeof(T) > size())
            throw ByteBufferPositionException(false, pos, sizeof(T), size());
        T val = *((T const*)&_storage[pos]);
        EndianConvert(val);
        return val;
    }

    void read(uint8* dest, size_t len)
    {
        if (_rpos  + len > size())
            throw ByteBufferPositionException(false, _rpos, len, size());
        std::memcpy(dest, &_storage[_rpos], len);
        _rpos += len;
    }

    void readPackGUID(uint64& guid)
    {
        if (rpos() + 1 > size())
            throw ByteBufferPositionException(false, _rpos, 1, size());

        guid = 0;

        uint8 guidmark = 0;
        (*this) >> guidmark;

        for (int i = 0; i < 8; ++i)
        {
            if (guidmark & (uint8(1) << i))
            {
                if (rpos() + 1 > size())
                    throw ByteBufferPositionException(false, _rpos, 1, size());

                uint8 bit;
                (*this) >> bit;
                guid |= (uint64(bit) << (i * 8));
            }
        }
    }

    uint32 ReadPackedTime()
    {
        uint32 packedDate = read<uint32>();
        tm lt = tm();

        lt.tm_min = packedDate & 0x3F;
        lt.tm_hour = (packedDate >> 6) & 0x1F;
        //lt.tm_wday = (packedDate >> 11) & 7;
        lt.tm_mday = ((packedDate >> 14) & 0x3F) + 1;
        lt.tm_mon = (packedDate >> 20) & 0xF;
        lt.tm_year = ((packedDate >> 24) & 0x1F) + 100;

        return uint32(mktime(&lt));

    }

    ByteBuffer& ReadPackedTime(uint32& time)
    {
        time = ReadPackedTime();
        return *this;
    }

    uint8* contents()
    {
        if (_storage.empty())
            throw ByteBufferException();
        return &_storage[0];
    }

    [[nodiscard]] const uint8* contents() const
    {
        if (_storage.empty())
            throw ByteBufferException();
        return &_storage[0];
    }

    [[nodiscard]] size_t size() const { return _storage.size(); }
    [[nodiscard]] bool empty() const { return _storage.empty(); }

    void resize(size_t newsize)
    {
        _storage.resize(newsize, 0);
        _rpos = 0;
        _wpos = size();
    }

    void reserve(size_t ressize)
    {
        if (ressize > size())
            _storage.reserve(ressize);
    }

    void append(const char* src, size_t cnt)
    {
        return append((const uint8*)src, cnt);
    }

    template<class T> void append(const T* src, size_t cnt)
    {
        return append((const uint8*)src, cnt * sizeof(T));
    }

    void append(const uint8* src, size_t cnt)
    {
        if (!cnt)
            throw ByteBufferSourceException(_wpos, size(), cnt);

        if (!src)
            throw ByteBufferSourceException(_wpos, size(), cnt);

        ASSERT(size() < 10000000);

        size_t newsize = _wpos + cnt;

        if (_storage.capacity() < newsize) // pussywizard
        {
            if (newsize < 100)
                _storage.reserve(300);
            else if (newsize < 750)
                _storage.reserve(2500);
            else if (newsize < 6000)
                _storage.reserve(10000);
            else
                _storage.reserve(400000);
        }

        if (_storage.size() < newsize)
            _storage.resize(newsize);

        memcpy(&_storage[_wpos], src, cnt);
        _wpos = newsize;
    }

    void append(const ByteBuffer& buffer)
    {
        if (buffer.wpos())
            append(buffer.contents(), buffer.wpos());
    }

    // can be used in SMSG_MONSTER_MOVE opcode
    void appendPackXYZ(float x, float y, float z)
    {
        uint32 packed = 0;
        packed |= ((int)(x / 0.25f) & 0x7FF);
        packed |= ((int)(y / 0.25f) & 0x7FF) << 11;
        packed |= ((int)(z / 0.25f) & 0x3FF) << 22;
        *this << packed;
    }

    void appendPackGUID(uint64 guid)
    {
        uint8 packGUID[8 + 1];
        packGUID[0] = 0;
        size_t size = 1;
        for (uint8 i = 0; guid != 0; ++i)
        {
            if (guid & 0xFF)
            {
                packGUID[0] |= uint8(1 << i);
                packGUID[size] =  uint8(guid & 0xFF);
                ++size;
            }

            guid >>= 8;
        }
        append(packGUID, size);
    }

    void AppendPackedTime(time_t time)
    {
        tm lt;
        localtime_r(&time, &lt);
        append<uint32>((lt.tm_year - 100) << 24 | lt.tm_mon  << 20 | (lt.tm_mday - 1) << 14 | lt.tm_wday << 11 | lt.tm_hour << 6 | lt.tm_min);
    }

    void put(size_t pos, const uint8* src, size_t cnt)
    {
        if (pos + cnt > size())
            throw ByteBufferPositionException(true, pos, cnt, size());

        if (!src)
            throw ByteBufferSourceException(_wpos, size(), cnt);

        std::memcpy(&_storage[pos], src, cnt);
    }

    void hexlike(bool outString = false) const;

protected:
    size_t _rpos{0}, _wpos{0};
    std::vector<uint8> _storage;
};

template <typename T>
inline ByteBuffer& operator<<(ByteBuffer& b, std::vector<T> v)
{
    b << (uint32)v.size();
    for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
inline ByteBuffer& operator>>(ByteBuffer& b, std::vector<T>& v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename T>
inline ByteBuffer& operator<<(ByteBuffer& b, std::list<T> v)
{
    b << (uint32)v.size();
    for (typename std::list<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
inline ByteBuffer& operator>>(ByteBuffer& b, std::list<T>& v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer& operator<<(ByteBuffer& b, std::map<K, V>& m)
{
    b << (uint32)m.size();
    for (typename std::map<K, V>::iterator i = m.begin(); i != m.end(); ++i)
    {
        b << i->first << i->second;
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer& operator>>(ByteBuffer& b, std::map<K, V>& m)
{
    uint32 msize;
    b >> msize;
    m.clear();
    while (msize--)
    {
        K k;
        V v;
        b >> k >> v;
        m.insert(make_pair(k, v));
    }
    return b;
}

/// @todo Make a ByteBuffer.cpp and move all this inlining to it.
template<> inline std::string ByteBuffer::read<std::string>()
{
    std::string tmp;
    *this >> tmp;
    return tmp;
}

template<>
inline void ByteBuffer::read_skip<char*>()
{
    std::string temp;
    *this >> temp;
}

template<>
inline void ByteBuffer::read_skip<char const*>()
{
    read_skip<char*>();
}

template<>
inline void ByteBuffer::read_skip<std::string>()
{
    read_skip<char*>();
}

#endif
