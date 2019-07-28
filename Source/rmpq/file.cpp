#include "file.h"
#include <set>
#include <algorithm>
#include <stdarg.h>

#ifndef NO_SYSTEM

#define NOMINMAX
#ifdef _MSC_VER
#include <windows.h>
#endif

void create_dir(char const* path) {
#ifdef _MSC_VER
  CreateDirectory(path, NULL);
#else
  struct stat st;
  if (stat(path, &st)) {
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }
#endif
}

class StdFileBuffer : public FileBuffer {
  FILE* file_;
public:
  StdFileBuffer(FILE* file)
    : file_(file)
  {}
  ~StdFileBuffer() {
    fclose(file_);
  }

  int getc() {
    return fgetc(file_);
  }
  void putc(int chr) {
    fputc(chr, file_);
  }

  uint64_t tell() const {
    return ftell(file_);
  }
  void seek(int64_t pos, int mode) {
    fseek(file_, (long) pos, mode);
  }

  size_t read(void* ptr, size_t size) {
    return fread(ptr, 1, size, file_);
  }
  size_t write(void const* ptr, size_t size) {
    return fwrite(ptr, 1, size, file_);
  }
};

File::File(char const* name, char const* mode) {
  FILE* file = fopen(name, mode);
  if (!file && (mode[0] == 'w' || mode[0] == 'a')) {
    std::string buf;
    for (int i = 0; name[i]; ++i) {
      char chr = name[i];
      if (chr == '/' || chr == '\\') chr = '\\';
      buf.push_back(chr);
      if (chr == '\\') {
        create_dir(buf.c_str());
      }
    }
    file = fopen(name, mode);
  }
  if (file) {
    file_ = std::make_shared<StdFileBuffer>(file);
  }
}

#endif

void File::printf(char const* fmt, ...) {
  static char buf[1024];

  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(nullptr, 0, fmt, ap);
  va_end(ap);
  char* dst;
  if (len < 1024) {
    dst = buf;
  } else {
    dst = new char[len + 1];
  }
  va_start(ap, fmt);
  vsnprintf(dst, len + 1, fmt, ap);
  va_end(ap);
  file_->write(dst, len);
  if (dst != buf) {
    delete[] dst;
  }
}
bool File::getline(std::string& out) {
  out.clear();
  int chr;
  while ((chr = file_->getc()) != EOF) {
    if (chr == '\r') {
      char next = file_->getc();
      if (next != '\n' && next != EOF) {
        file_->seek(-1, SEEK_CUR);
      }
      return true;
    }
    if (chr == '\n') {
      return true;
    }
    out.push_back(chr);
  }
  return !out.empty();
}
bool File::getwline(std::wstring& out) {
  out.clear();
  wchar_t chr;
  while (read(&chr, sizeof chr) == 2) {
    if (chr == L'\r') {
      wchar_t next;
      uint64_t pos = file_->tell();
      if (read(&next, sizeof next) != 2 || next != L'\n') {
        file_->seek(pos, SEEK_SET);
      }
      return true;
    }
    if (chr == L'\n') {
      return true;
    }
    out.push_back(chr);
  }
  return !out.empty();
}
bool File::getwline_flip(std::wstring& out) {
  out.clear();
  wchar_t chr;
  while (read(&chr, sizeof chr) == 2) {
    chr = ((chr & 0xFF00) >> 8) | ((chr & 0x00FF) << 8);
    if (chr == L'\r') {
      wchar_t next;
      uint64_t pos = file_->tell();
      if (read(&next, sizeof next) != 2 || next != 0x0A00) {
        file_->seek(pos, SEEK_SET);
      }
      return true;
    }
    if (chr == L'\n') {
      return true;
    }
    out.push_back(chr);
  }
  return !out.empty();
}

File::LineIterator<std::string> File::begin() {
  return LineIterator<std::string>(*this, &File::getline);
}
File::LineIterator<std::string> File::end() {
  return LineIterator<std::string>();
}
File::LineIterator<std::wstring> File::wbegin() {
  int bom = read16();
  if (bom == 0xFEFF) {
    return LineIterator<std::wstring>(*this, &File::getwline);
  } else if (bom == 0xFFFE) {
    return LineIterator<std::wstring>(*this, &File::getwline_flip);
  } else {
    seek(-2, SEEK_CUR);
    return LineIterator<std::wstring>(*this, &File::getwline);
  }
}
File::LineIterator<std::wstring> File::wend() {
  return LineIterator<std::wstring>();
}

class SubFileBuffer : public FileBuffer {
  File file_;
  uint64_t start_;
  uint64_t end_;
  uint64_t pos_;
public:
  SubFileBuffer(File& file, uint64_t offset, uint64_t size)
    : file_(file)
    , start_(offset)
    , end_(offset + size)
    , pos_(offset)
  {
    file.seek(offset);
  }

  int getc() {
    if (pos_ >= end_) return EOF;
    ++pos_;
    return file_.getc();
  }
  void putc(int chr) {}

  uint64_t tell() const {
    return pos_ - start_;
  }
  void seek(int64_t pos, int mode) {
    switch (mode) {
    case SEEK_SET:
      pos += start_;
      break;
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += end_;
      break;
    }
    if (static_cast<uint64_t>(pos) < start_) pos = start_;
    if (static_cast<uint64_t>(pos) > end_) pos = end_;
    pos_ = pos;
    file_.seek(pos_);
  }
  uint64_t size() {
    return end_ - start_;
  }

  size_t read(void* ptr, size_t size) {
    if (size + pos_ > end_) {
      size = (size_t)(end_ - pos_);
    }
    if (size) {
      size = file_.read(ptr, size);
      pos_ += size;
    }
    return size;
  }
  size_t write(void const* ptr, size_t size) {
    return 0;
  }
};

File File::subfile(uint64_t offset, uint64_t size) {
  return File(std::make_shared<SubFileBuffer>(*this, offset, size));
}

class MemoryBuffer : public FileBuffer {
  size_t pos_ = 0;
  std::vector<uint8_t> data_;
public:
  MemoryBuffer()
  {}
  MemoryBuffer(std::vector<uint8_t> const& data)
    : data_(data)
  {}
  MemoryBuffer(std::vector<uint8_t>&& data)
    : data_(std::move(data))
  {}

  int getc() {
    return (pos_ < data_.size() ? data_[pos_++] : EOF);
  }
  void putc(int chr) {
    if (pos_ >= data_.size()) {
      data_.push_back(chr);
    } else {
      data_[pos_] = chr;
    }
    pos_++;
  }

  uint64_t tell() const {
    return pos_;
  }
  void seek(int64_t pos, int mode) {
    switch (mode) {
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += data_.size();
      break;
    }
    if (pos < 0) pos = 0;
    if (static_cast<size_t>(pos) > data_.size()) pos = data_.size();
    pos_ = (size_t) pos;
  }
  uint64_t size() {
    return data_.size();
  }

  size_t read(void* ptr, size_t size) {
    if (size + pos_ > data_.size()) {
      size = data_.size() - pos_;
    }
    if (size) {
      memcpy(ptr, data_.data() + pos_, size);
      pos_ += size;
    }
    return size;
  }

  size_t write(void const* ptr, size_t size) {
    memcpy(alloc(size), ptr, size);
    return size;
  }

  uint8_t const* data() const {
    return data_.data();
  }

  void resize(size_t size) {
    data_.resize(size);
  }

  uint8_t* alloc(size_t size) {
    if (size + pos_ > data_.size()) {
      data_.resize(size + pos_);
    }
    uint8_t* result = data_.data() + pos_;
    pos_ += size;
    return result;
  }
};

class RawMemoryBuffer : public FileBuffer {
  uint8_t const* data_;
  size_t size_;
  size_t pos_ = 0;
public:
  RawMemoryBuffer(void const* data, size_t size)
    : data_(reinterpret_cast<uint8_t const*>(data))
    , size_(size)
  {}

  int getc() {
    return (pos_ < size_ ? data_[pos_++] : EOF);
  }
  void putc(int chr) {};

  uint64_t tell() const {
    return pos_;
  }
  void seek(int64_t pos, int mode) {
    switch (mode) {
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += size_;
      break;
    }
    if (pos < 0) pos = 0;
    if (static_cast<size_t>(pos) > size_) pos = size_;
    pos_ = (size_t) pos;
  }
  uint64_t size() {
    return size_;
  }

  size_t read(void* ptr, size_t size) {
    if (size + pos_ > size_) {
      size = size_ - pos_;
    }
    if (size) {
      memcpy(ptr, data_ + pos_, size);
      pos_ += size;
    }
    return size;
  }

  size_t write(void const* ptr, size_t size) {
    return 0;
  }

  uint8_t const* data() const {
    return data_;
  }
};

MemoryFile::MemoryFile()
  : File(std::make_shared<MemoryBuffer>())
{}
MemoryFile::MemoryFile(std::vector<uint8_t> const& data)
  : File(std::make_shared<MemoryBuffer>(data))
{}
MemoryFile::MemoryFile(std::vector<uint8_t>&& data)
  : File(std::make_shared<MemoryBuffer>(std::move(data)))
{}
MemoryFile::MemoryFile(void const* data, size_t size)
  : File(std::make_shared<RawMemoryBuffer>(data, size))
{}
MemoryFile::MemoryFile(File const& file)
  : File(std::dynamic_pointer_cast<MemoryBuffer>(file.buffer()))
{}

MemoryFile MemoryFile::from(File file) {
  MemoryFile mem(file);
  if (mem) {
    return mem;
  }
  auto pos = file.tell();
  std::vector<uint8_t> buffer((size_t) file.size());
  file.seek(0);
  file.read(buffer.data(), buffer.size());
  mem = MemoryFile(std::move(buffer));
  file.seek(pos);
  mem.seek(pos);
  return mem;
}

uint8_t const* MemoryFile::data() const {
  MemoryBuffer* buffer = dynamic_cast<MemoryBuffer*>(file_.get());
  if (buffer) return buffer->data();
  RawMemoryBuffer* rawBuffer = dynamic_cast<RawMemoryBuffer*>(file_.get());
  return (rawBuffer ? rawBuffer->data() : nullptr);
}
uint8_t* MemoryFile::alloc(size_t size) {
  MemoryBuffer* buffer = dynamic_cast<MemoryBuffer*>(file_.get());
  return (buffer ? buffer->alloc(size) : nullptr);
}
void MemoryFile::resize(size_t size) {
  MemoryBuffer* buffer = dynamic_cast<MemoryBuffer*>(file_.get());
  if (buffer) buffer->resize(size);
}

void File::copy(File src, uint64_t size) {
  auto mem = dynamic_cast<MemoryBuffer*>(src.file_.get());
  if (mem) {
    uint64_t pos = mem->tell();
    size = std::min(size, mem->size() - pos);
    write(mem->data() + pos, (size_t) size);
    mem->seek(size, SEEK_CUR);
  } else {
    uint8_t buf[65536];
    while (size_t count = src.read(buf, std::min<size_t>(sizeof buf, (size_t) size))) {
      write(buf, count);
      size -= count;
    }
  }
}

#ifndef NO_SYSTEM
#include <sys/stat.h>

bool File::exists(char const* path) {
  struct stat buffer;
  return (stat(path, &buffer) == 0);
}

#endif

#include "checksum.h"
void File::md5(void* digest) {
  auto mem = dynamic_cast<MemoryBuffer*>(file_.get());
  if (mem) {
    MD5::checksum(mem->data(), (uint32_t) mem->size(), digest);
  } else {
    uint64_t pos = tell();
    seek(0, SEEK_SET);
    uint8_t buf[65536];
    MD5 checksum;
    while (size_t count = read(buf, sizeof buf)) {
      checksum.process(buf, count);
    }
    checksum.finish(digest);
    seek(pos, SEEK_SET);
  }
}
