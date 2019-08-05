#pragma once

#include <string>
#include <memory>
#include <vector>

class FileBuffer {
public:
  virtual ~FileBuffer() {}

  virtual int getc() {
    uint8_t chr;
    if (read(&chr, 1) != 1) {
      return EOF;
    }
    return chr;
  }
  virtual void putc(int chr) {
    write(&chr, 1);
  }

  virtual uint64_t tell() const = 0;
  virtual void seek(int64_t pos, int mode) = 0;
  virtual uint64_t size() {
    uint64_t pos = tell();
    seek(0, SEEK_END);
    uint64_t res = tell();
    seek(pos, SEEK_SET);
    return res;
  }

  virtual size_t read(void* ptr, size_t size) = 0;
  virtual size_t write(void const* ptr, size_t size) = 0;

  virtual void truncate() = 0;
};

class File {
protected:
  std::shared_ptr<FileBuffer> file_;
public:
  File()
  {}
  File(std::shared_ptr<FileBuffer> const& file)
    : file_(file)
  {}
  File(File const& file)
    : file_(file.file_)
  {}
  File(File&& file)
    : file_(std::move(file.file_))
  {}

  explicit File(char const* name, char const* mode = "rb");
  explicit File(std::string const& name, char const* mode = "rb")
    : File(name.c_str(), mode)
  {}

  void release() {
    file_.reset();
  }

  File& operator=(File const& file) {
    if (file_ == file.file_) {
      return *this;
    }
    file_ = file.file_;
    return *this;
  }
  File& operator=(File&& file) {
    if (file_ == file.file_) {
      return *this;
    }
    file_ = std::move(file.file_);
    return *this;
  }

  operator bool() const {
    return file_ != nullptr;
  }

  int getc() {
    return file_->getc();
  }
  void putc(int chr) {
    file_->putc(chr);
  }

  void seek(int64_t pos, int mode = SEEK_SET) {
    file_->seek(pos, mode);
  }
  uint64_t tell() const {
    return file_->tell();
  }
  uint64_t size() {
    return file_->size();
  }

  size_t read(void* dst, size_t size) {
    return file_->read(dst, size);
  }
  template<class T>
  T read() {
    T x;
    file_->read(&x, sizeof(T));
    return x;
  }
  uint8_t read8() {
    uint8_t x;
    file_->read(&x, 1);
    return x;
  }
  uint16_t read16(bool big = false) {
    uint16_t x;
    file_->read(&x, 2);
    if (big) {
      x = ((x & 0xFF00) >> 8) | ((x & 0x00FF) << 8);
    }
    return x;
  }
  uint32_t read32(bool big = false) {
    uint32_t x;
    file_->read(&x, 4);
    if (big) {
      x = ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24);
    }
    return x;
  }

  uint64_t read64(bool big = false) {
    uint64_t x;
    file_->read(&x, 8);
    if (big) {
      x = ((x & 0xFF00000000000000ULL) >> 56) | ((x & 0x00FF000000000000ULL) >> 40) | ((x & 0x0000FF0000000000ULL) >> 24) | ((x & 0x000000FF00000000ULL) >> 8) |
        ((x & 0x00000000FF000000ULL) << 8) | ((x & 0x0000000000FF0000ULL) << 24) | ((x & 0x000000000000FF00ULL) << 40) | ((x & 0x00000000000000FFULL) << 56);
    }
    return x;
  }

  size_t write(void const* ptr, size_t size) {
    return file_->write(ptr, size);
  }
  template<class T>
  bool write(T const& x) {
    return file_->write(&x, sizeof(T)) == sizeof(T);
  }
  bool write8(uint8_t x) {
    return file_->write(&x, 1) == 1;
  }
  bool write16(uint16_t x, bool big = false) {
    if (big) {
      x = ((x & 0xFF00) >> 8) | ((x & 0x00FF) << 8);
    }
    return file_->write(&x, 2) == 2;
  }
  bool write32(uint32_t x, bool big = false) {
    if (big) {
      x = ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24);
    }
    return file_->write(&x, 4) == 4;
  }
  bool write64(uint64_t x, bool big = false) {
    if (big) {
      x = ((x & 0xFF00000000000000ULL) >> 56) | ((x & 0x00FF000000000000ULL) >> 40) | ((x & 0x0000FF0000000000ULL) >> 24) | ((x & 0x000000FF00000000ULL) >> 8) |
        ((x & 0x00000000FF000000ULL) << 8) | ((x & 0x0000000000FF0000ULL) << 24) | ((x & 0x000000000000FF00ULL) << 40) | ((x & 0x00000000000000FFULL) << 56);
    }
    return file_->write(&x, 8) == 8;
  }

  void truncate() {
    file_->truncate();
  }

  void printf(char const* fmt, ...);

  bool getline(std::string& line);
  bool getwline(std::wstring& line);
  bool getwline_flip(std::wstring& line);

  template<class string_t>
  class LineIterator;

  LineIterator<std::string> begin();
  LineIterator<std::string> end();

  LineIterator<std::wstring> wbegin();
  LineIterator<std::wstring> wend();

  File subfile(uint64_t offset, uint64_t size);

  void copy(File src, uint64_t size = std::numeric_limits<uint64_t>::max());
  void md5(void* digest);

  std::shared_ptr<FileBuffer> buffer() const {
    return file_;
  }

  static void remove(const char* path);

#ifndef NO_SYSTEM
  static bool exists(char const* path);
  static bool exists(std::string const& path) {
    return exists(path.c_str());
  }
#endif
};

class MemoryFile : public File {
public:
  MemoryFile();
  MemoryFile(std::vector<uint8_t> const& data);
  MemoryFile(std::vector<uint8_t>&& data);
  MemoryFile(void const* data, size_t size);
  MemoryFile(File const& file);

  static MemoryFile from(File file);

  uint8_t const* data() const;
  uint8_t* alloc(size_t size);
  void resize(size_t size);
};

template<class string_t>
class File::LineIterator {
  friend class File;
  typedef bool(File::*getter_t)(string_t& line);
  File file_;
  string_t line_;
  getter_t getter_ = nullptr;
  LineIterator(File& file, getter_t getter)
    : getter_(getter)
  {
    if ((file.*getter_)(line_)) {
      file_ = file;
    }
  }
public:
  LineIterator() {}

  string_t const& operator*() {
    return line_;
  }
  string_t const* operator->() {
    return &line_;
  }

  bool operator!=(LineIterator const& it) const {
    return file_ != it.file_;
  }

  bool valid() {
    return file_;
  }

  LineIterator& operator++() {
    if (!(file_.*getter_)(line_)) {
      file_.release();
    }
    return *this;
  }
};

class WideFile {
public:
  WideFile(File const& file)
    : file_(file)
  {}

  File::LineIterator<std::wstring> begin() {
    return file_.wbegin();
  }
  File::LineIterator<std::wstring> end() {
    return file_.wend();
  }
private:
  File file_;
};

class FileLoader {
public:
  virtual ~FileLoader() {};

  virtual File load(char const* path) = 0;
};
