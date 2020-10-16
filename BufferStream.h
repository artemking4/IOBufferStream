#pragma once

#ifdef BUFSIZE_64
typedef unsigned __int64 bufsize_t;
#else
typedef unsigned __int32 bufsize_t;
#endif

#ifndef BUF_SIZE_DEFAULT
#define BUF_SIZE_DEFAULT 512
#endif

class IOBufferStream;

class IBufSerializable {
    virtual void Serialize(IOBufferStream* buf);
    virtual void Deserialize(IOBufferStream* buf);
};

struct BufferStream : std::streambuf
{
protected:
    std::vector<char> _buf = { };

    void _bufresize(bufsize_t len, bool resetCursors = false) {
        std::vector<char> newbuf(len);
        char* data = _buf.data();
        size_t datalen = _buf.size();
        std::copy(data, data + datalen, newbuf.data());

        if (!resetCursors) {
            this->setg(newbuf.data(), newbuf.data() + (this->gptr() - data), newbuf.data() + newbuf.size()); // Alright, im too lazy to put this shit into variables so punch me in my face later kthxbai
            this->setp(newbuf.data(), newbuf.data() + (this->pptr() - data), newbuf.data() + newbuf.size());
        }
        else {
            this->setg(newbuf.data(), newbuf.data(), newbuf.data() + newbuf.size());
            this->setp(newbuf.data(), newbuf.data(), newbuf.data() + newbuf.size());
        }

        _buf.swap(newbuf);
    }

public:
    BufferStream(char* begin, bufsize_t len) {
        _bufresize(len, true);
        std::memcpy(begin, _buf.data(), len);
    }

    BufferStream(bufsize_t len = BUF_SIZE_DEFAULT) {
        if (len == 0)
            return;

        _bufresize(len, true);
    }

    inline char* Data() const {
        return (char*) _buf.data();
    }

    inline bufsize_t Size() const {
        return _buf.size();
    }

    inline char* CursorPtr(bool read = false) const {
        char* ptr = (read) ? this->gptr() : this->pptr();
        return ptr;
    }

    inline void AdvanceCursor(size_t len, bool read = false) {
        if (read)
            this->setg(_buf.data(), this->gptr() + len, _buf.data() + _buf.size());
        else
            this->setp(_buf.data(), this->pptr() + len, _buf.data() + _buf.size());
    }

    inline bufsize_t BytesLeft(bool read = false) const {
        bufsize_t left = _buf.data() + _buf.size() - ((read) ? this->gptr() : this->pptr());
        return left;
    }

    inline void Reserve(bufsize_t bytes) {
        _bufresize(_buf.size() + bytes);
    }

    inline void Resize(bufsize_t bytes) {
        _bufresize(bytes);
    }
};

struct IOBufferStream : std::iostream 
{
private:
    BufferStream _stream;

public:
    inline char* Data() const {
        return _stream.Data();
    }

    inline bufsize_t Size() const {
        return _stream.Size();
    }

    IOBufferStream(bufsize_t len = BUF_SIZE_DEFAULT) : _stream(len), std::iostream(&_stream) { };
    IOBufferStream(char* begin, bufsize_t len) : _stream(begin, len), std::iostream(&_stream) { };
    IOBufferStream(IOBufferStream&& old) : _stream((char*) old.Data(), old.Size()), std::iostream(&_stream) { };
    IOBufferStream(const IOBufferStream& old) : _stream((char*) old.Data(), old.Size()), std::iostream(&_stream) { };
    void operator=(IOBufferStream right) { 
        _stream.Resize(right.Size());
        std::memcpy((char*) right.Data(), _stream.Data(), right.Size());
    };

    inline char* CursorPtr(bool read = false) const {
        return _stream.CursorPtr(read);
    }

    inline void AdvanceCursor(size_t len, bool read = false) {
        _stream.AdvanceCursor(len, read);
    }

    inline bufsize_t BytesLeft(bool read = false) const {
        return _stream.BytesLeft(read);
    }

    inline void Reserve(bufsize_t s) {
        _stream.Reserve(s);
    }

    inline void Resize(bufsize_t s) {
        _stream.Resize(s);
    }

    template <typename T>
    inline T Read() {
        T var;
        
        if (!this->read((char*)&var, sizeof(T)))
            throw std::out_of_range("Failed reading bytes! Maybe the buffer is too small.");

        return var;
    }

    template <typename T>
    inline void Read(T& var) {
        if (!this->read((char*)&var, sizeof(T)))
            throw std::out_of_range("Failed reading bytes! Maybe the buffer is too small.");
    }

    template <typename T>
    inline void ReadArray(T* arr, unsigned int len = 0) {
        len = len == 0 ? Read<unsigned int>() : len;

        if (!this->read((char*)arr, len * sizeof(T)))
            throw std::out_of_range("Failed reading bytes! Maybe the buffer is too small.");
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

    inline void ReadString(std::string& str, unsigned int len = 0) {
        len = len == 0 ? Read<unsigned int>() : len;

        str.resize(len);
        if (!this->read((char*)str.c_str(), len))
            throw std::out_of_range("Failed reading bytes! Maybe the buffer is too small.");
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

    template<typename T>
    inline IOBufferStream& operator>>(T& what) { what = Read<T>(); return *this; }
    template<typename T>
    inline IOBufferStream& operator<<(T what) { Write<T>(what); return *this; }

    inline IOBufferStream& operator<<(const char* what) { WriteString(what); return *this; }

    inline IOBufferStream& operator>>(std::string& what) { what = ReadString(); return *this; }
    inline IOBufferStream& operator<<(std::string what) { WriteString(what); return *this; }
};
