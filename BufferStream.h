#pragma once

#ifdef BUFSIZE_64
typedef unsigned __int64 bufsize_t;
#else
typedef unsigned __int32 bufsize_t;
#endif

constexpr bufsize_t BUF_SIZE_DEFAULT = 512;

struct BufferStream : std::streambuf
{
private:
    char* _buf = 0;
    bufsize_t _buflen = 0;

    void _bufresize(bufsize_t len) {
        auto oldbuf = _buf;
        auto oldbuflen = _buflen;

        char* newbuf = new char[len];

        if (oldbuf) {
            std::copy(oldbuf, oldbuf + oldbuflen, newbuf);
        }

        _buf = newbuf;
        _buflen = len;

        this->setg(_buf, _buf + (this->gptr() - oldbuf), _buf + _buflen);
        this->setp(_buf, _buf + (this->pptr() - oldbuf), _buf + _buflen);

        if(oldbuf)
            delete[] oldbuf;
    }

public:
    BufferStream(const char* begin, bufsize_t len) {
        _bufresize(len);
        std::memcpy(_buf, begin, len);
    }

    BufferStream(bufsize_t len = BUF_SIZE_DEFAULT) {
        _bufresize(len);
    }

    inline char* Data() {
        return _buf;
    }

    inline bufsize_t Size() {
        return _buflen;
    }

    inline bufsize_t BytesLeft(bool read = false) {
        bufsize_t left = _buf + _buflen - ((read) ? this->gptr() : this->pptr());
        return left;
    }

    void Reserve(bufsize_t bytes) {
        _bufresize(_buflen + bytes);
    }
};

struct IOBufferStream : std::iostream 
{
private:
    BufferStream _stream;

public:
    IOBufferStream(bufsize_t len = BUF_SIZE_DEFAULT) : _stream(len), std::iostream(&_stream) { };
    IOBufferStream(const char* begin, bufsize_t len) : _stream(begin, len), std::iostream(&_stream) { };

    inline char* Data() {
        return _stream.Data();
    }

    inline bufsize_t Size() {
        return _stream.Size();
    }

    inline bufsize_t BytesLeft(bool read = false) {
        return _stream.BytesLeft();
    }

    template <typename T>
    inline T Read() {
        T var;
        
        if (!this->read((char*)&var, sizeof(T)))
            throw std::out_of_range("Failed reading bytes! Maybe the buffer is too small.");

        return var;
    }

    template <typename T>
    inline std::vector<T> ReadArray(unsigned int len = 0) {
        len = len == 0 ? Read<unsigned int>() : len;

        std::vector<T> arr;
        arr.resize(len);
        if (!this->read((char*)arr.data(), len * sizeof(T)))
            throw std::out_of_range("Failed reading bytes! Maybe the buffer is too small.");

        return arr;
    }

    inline std::string ReadString(unsigned int len = 0) {
        len = len == 0 ? Read<unsigned int>() : len;

        std::string str;
        str.resize(len);
        if (!this->read((char*)str.c_str(), len))
            throw std::out_of_range("Failed reading bytes! Maybe the buffer is too small.");

        return str;
    }

    template <typename T>
    inline void Write(T var) {
        if (BytesLeft() < sizeof(T))
            _stream.Reserve(sizeof(T) - BytesLeft());

        if (!this->write((const char*)&var, sizeof(T)))
            throw std::exception("Failed writing bytes! Maybe the buffer allocation failed.");
    }

    template <typename T>
    inline void WriteArray(T* arr, unsigned int len, bool writelen = true) {
        if (writelen)
            Write<unsigned int>(len);
        
        if (BytesLeft() < len * sizeof(T))
            _stream.Reserve((len * sizeof(T)) - BytesLeft());

        if (!this->write((const char*)arr, len * sizeof(T)))
            throw std::exception("Failed writing bytes! Maybe the buffer allocation failed.");
    }

    inline void WriteString(std::string str, unsigned int len = 0, bool writelen = true) {
        len = (len == 0) ? str.length() : len;

        if (writelen)
            Write<unsigned int>(len);

        if (BytesLeft() < len)
            _stream.Reserve(len - BytesLeft());

        if (!this->write(str.c_str(), len))
            throw std::exception("Failed writing bytes! Maybe the buffer allocation failed.");
    }
};