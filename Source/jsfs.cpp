#include "rmpq/file.h"
#include <algorithm>

#ifdef EMSCRIPTEN

#include <emscripten.h>

EM_JS( int, get_file_size, (const char* path), {
  var end = HEAPU8.indexOf( 0, path);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(path, end ));
  return self.DApi.get_file_size(text);
});

EM_JS( void, get_file_contents, (const char* path, void* ptr, size_t offset, size_t size), {
  var end = HEAPU8.indexOf( 0, path);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(path, end ));
  self.DApi.get_file_contents(text, HEAPU8.subarray(ptr, ptr + size), offset);
});

EM_JS( void, put_file_contents, (const char* path, void* ptr, size_t size), {
  var end = HEAPU8.indexOf( 0, path);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(path, end));
  self.DApi.put_file_contents(text, HEAPU8.slice(ptr, ptr + size));
});

EM_JS( void, remove_file, (const char* path), {
  var end = HEAPU8.indexOf( 0, path);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(path, end ));
  self.DApi.remove_file( text );
});

void File::remove(const char* path) {
  remove_file(path);
}

#else

#include <io.h>

int get_file_size(const char* path) {
  struct stat st;
  if (stat(path, &st)) {
    return 0;
  }
  return (int) st.st_size;
}

void get_file_contents(const char* path, void* ptr, size_t offset, size_t size) {
  FILE* f = fopen(path, "rb");
  fseek(f, offset, SEEK_SET);
  fread(ptr, 1, size, f);
  fclose(f);
}

void put_file_contents(const char* path, void* ptr, size_t size) {
  FILE* f = fopen(path, "wb");
  fwrite(ptr, 1, size, f);
  fclose(f);
}

void File::remove(const char* path) {
  ::remove(path);
}

#endif

class JsMemoryBuffer : public FileBuffer {
  size_t pos_ = 0;
  std::vector<uint8_t> data_;
  bool modified = false;
public:
  std::string name;

  JsMemoryBuffer() {
  }
  JsMemoryBuffer(std::vector<uint8_t>&& data)
    : data_(std::move(data)) {
  }

  ~JsMemoryBuffer() {
    if (modified && !name.empty()) {
      put_file_contents(name.c_str(), data_.data(), data_.size());
    }
  }

  int getc() {
    return (pos_ < data_.size() ? data_[pos_++] : EOF);
  }
  void putc(int chr) {
    modified = true;
    if (pos_ >= data_.size()) {
      data_.resize(pos_ + 1);
      data_[pos_] = chr;
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
    //if (static_cast<size_t>(pos) > data_.size()) pos = data_.size();
    pos_ = (size_t) pos;
  }
  uint64_t size() {
    return data_.size();
  }

  size_t read(void* ptr, size_t size) {
    if (pos_ >= data_.size()) {
      return 0;
    }
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
    modified = true;
    memcpy(alloc(size), ptr, size);
    return size;
  }

  uint8_t* alloc(size_t size) {
    if (size + pos_ > data_.size()) {
      data_.resize(size + pos_);
    }
    uint8_t* result = data_.data() + pos_;
    pos_ += size;
    return result;
  }

  void truncate() {
    modified = true;
    data_.resize(pos_, 0);
  }
};

class ReadOnlyFile : public FileBuffer {
  size_t pos_ = 0;
  std::vector<uint8_t> data_;
  std::string path_;
  size_t size_;
  size_t loadPos_ = 0;
public:
  static const size_t MAX_CHUNK = 1 << 21;

  ReadOnlyFile(char const* path, size_t size)
    : path_(path)
    , size_(size) {
  }

  int getc() {
    if (pos_ >= size_) {
      return EOF;
    }
    if (pos_ >= loadPos_ && pos_ < loadPos_ + data_.size()) {
      return data_[pos_++ - loadPos_];
    }
    loadPos_ = pos_;
    data_.resize(std::min(MAX_CHUNK, size_ - pos_));
    get_file_contents(path_.c_str(), data_.data(), loadPos_, data_.size());
    return data_[pos_++ - loadPos_];
  }
  void putc(int chr) {
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
      pos += size_;
      break;
    }
    if (pos < 0) pos = 0;
    //if (static_cast<size_t>(pos) > data_.size()) pos = data_.size();
    pos_ = (size_t) pos;
  }
  uint64_t size() {
    return size_;
  }

  size_t read(void* ptr, size_t size) {
    if (pos_ >= size_) {
      return 0;
    }
    if (size + pos_ > size_) {
      size = size_ - pos_;
    }
    if (pos_ >= loadPos_ && pos_ + size <= loadPos_ + data_.size()) {
      memcpy(ptr, data_.data() + pos_ - loadPos_, size);
      pos_ += size;
      return size;
    }
    if (size <= MAX_CHUNK) {
      loadPos_ = pos_;
      data_.resize(std::min(MAX_CHUNK, size_ - pos_));
      get_file_contents(path_.c_str(), data_.data(), loadPos_, data_.size());
      memcpy(ptr, data_.data(), size);
      pos_ += size;
      return size;
    }
    get_file_contents(path_.c_str(), ptr, pos_, size);
    pos_ += size;
    return size;
  }

  size_t write(void const* ptr, size_t size) {
    return 0;
  }

  void truncate() {}
};

File::File(const char* name, const char* mode) {
  if (mode[0] == 'r') {
    size_t size = get_file_size(name);
    if (!size) {
      return;
    }
    if (size >= ReadOnlyFile::MAX_CHUNK && !strchr(mode, '+')) {
      file_ = std::make_shared<ReadOnlyFile>(name, size);
      return;
    }
    std::vector<uint8_t> data(size);
    get_file_contents(name, data.data(), 0, size);
    auto buffer = std::make_shared<JsMemoryBuffer>(std::move(data));
    file_ = buffer;
    if (strchr(mode, '+')) {
      buffer->name = name;
    }
  } else if (mode[0] == 'w') {
    std::vector<uint8_t> data;
    if (strchr(mode, '+')) {
      size_t size = get_file_size(name);
      if (size) {
        data.resize(size);
        get_file_contents(name, data.data(), 0, size);
      }
    }
    auto buffer = std::make_shared<JsMemoryBuffer>(std::move(data));
    file_ = buffer;
    buffer->name = name;
  }
}

#ifndef EMSCRIPTEN

#include <io.h>

class StdFileBuffer : public FileBuffer {
  FILE* file_;
public:
  StdFileBuffer(FILE* file)
    : file_(file) {
  }
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

  void truncate() {
    _chsize(_fileno(file_), (long) ftell(file_));
  }
};

//File::File(const char* name, const char* mode) {
//  FILE* file = fopen(name, mode);
//  if (file) {
//    file_ = std::make_shared<StdFileBuffer>(file);
//  }
//}

#endif
