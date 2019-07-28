#include "rmpq/file.h"

#ifdef EMSCRIPTEN

#include <emscripten.h>

EM_JS( int, get_file_size, (const char* path), {
  var end = HEAPU8.indexOf( 0, path);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(path, end ));
  return window.DApi.get_file_size(text);
});

EM_JS( void, get_file_contents, (const char* path, void* ptr, size_t size), {
  var end = HEAPU8.indexOf( 0, path);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(path, end ));
  window.DApi.get_file_contents(text, HEAPU8.subarray(ptr, ptr + size));
});

EM_JS( void, put_file_contents, (const char* path, void* ptr, size_t size), {
  var end = HEAPU8.indexOf( 0, path);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(path, end ));
  window.DApi.put_file_contents(text, HEAPU8.slice(ptr, ptr + size));
});

EM_JS( void, remove_file, (const char* path), {
  var end = HEAPU8.indexOf( 0, path);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(path, end ));
  window.DApi.remove_file( text );
});

class MemoryBuffer : public FileBuffer {
  size_t pos_ = 0;
  std::vector<uint8_t> data_;
  bool modified = false;
public:
  std::string name;

  MemoryBuffer() {
  }
  MemoryBuffer(std::vector<uint8_t>&& data)
    : data_(std::move(data)) {
  }

  ~MemoryBuffer() {
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
};

File::File(const char* name, const char* mode) {
  if (mode[0] == 'r') {
    size_t size = get_file_size(name);
    if (!size) {
      return;
    }
    std::vector<uint8_t> data(size);
    get_file_contents(name, data.data(), size);
    auto buffer = std::make_shared<MemoryBuffer>(std::move(data));
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
        get_file_contents(name, data.data(), size);
      }
    }
    auto buffer = std::make_shared<MemoryBuffer>(std::move(data));
    file_ = buffer;
    buffer->name = name;
  }
}

void File::remove(const char* path) {
  remove_file(path);
}

#else

int get_file_size(const char* path) {
  struct stat st;
  if (stat(path, &st)) {
    return 0;
  }
  return (int)st.st_size;
}

void get_file_contents(const char* path, void* ptr, size_t size) {
  FILE* f = fopen(path, "rb");
  fread(ptr, 1, size, f);
  fclose(f);
}

void put_file_contents(const char* path, void* ptr, size_t size) {
  FILE* f = fopen(path, "wb");
  fwrite(ptr, 1, size, f);
  fclose(f);
}

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
};

File::File(const char* name, const char* mode) {
  FILE* file = fopen(name, mode);
  if (file) {
    file_ = std::make_shared<StdFileBuffer>(file);
  }
}

void File::remove(const char* path) {
  ::remove(path);
}

#endif
