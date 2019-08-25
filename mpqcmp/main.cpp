#include <stdint.h>
#include <algorithm>
#include <set>
#include <string>
typedef unsigned long DWORD;
typedef void* HWND;
typedef int BOOL;
#include "../Source/appfat.h"
#include "../Source/rmpq/file.h"
#include "../Source/rmpq/archive.h"
#include "../Source/rmpq/common.h"

//#define MULTITHREADING

#ifdef MULTITHREADING
#include <atomic>
#include <thread>
#include <mutex>
typedef std::mutex mutex_t;
#else
typedef int mutex_t;
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
EM_JS(void, do_error, (const char* err), {
  var end = HEAPU8.indexOf(0, err);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(err, end));
  self.DApi.exit_error(text);
});
EM_JS(void, do_progress, (int done, int total), {
  self.DApi.progress(done, total);
});
#else
void do_error(const char* err) {
  printf("%s\r\n", err);
  exit(0);
}
void do_progress(int done, int total) {
  printf("\r%d / %d", done, total);
}
#endif

void ErrMsg(const char* text, const char *log_file_path, int log_line_nr) {
  do_error(text);
}

#ifdef EMSCRIPTEN

EM_JS(void, get_file_contents, (void* ptr, size_t offset, size_t size), {
  self.DApi.get_file_contents(HEAPU8.subarray(ptr, ptr + size), offset);
});

EM_JS(void, put_file_size, (size_t size), {
  self.DApi.put_file_size(size);
});

EM_JS(void, put_file_contents, (void* ptr, size_t offset, size_t size), {
  self.DApi.put_file_contents(HEAPU8.subarray(ptr, ptr + size), offset);
});

#else

File __input;
File __output;

void get_file_contents(void* ptr, size_t offset, size_t size) {
  __input.seek(offset);
  __input.read(ptr, size);
}

void put_file_size(size_t size) {
  __output.seek(size);
  __output.truncate();
}

void put_file_contents(const void* ptr, size_t offset, size_t size) {
  __output.seek(offset);
  __output.write(ptr, size);
}

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
File::File(const char* name, const char* mode) {
  FILE* file = fopen(name, mode);
  if (file) {
    file_ = std::make_shared<StdFileBuffer>(file);
  }
}

#endif

class RandomAccessBuffer : public FileBuffer {
public:
  RandomAccessBuffer(size_t size = 0)
    : size_(size) {
  }

  uint64_t tell() const override {
    return pos_;
  }
  void seek(int64_t pos, int mode) override {
    switch (mode) {
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += size_;
      break;
    }
    if (pos < 0) pos = 0;
    pos_ = (size_t) pos;
  }
  uint64_t size() override {
    return size_;
  }

protected:
  size_t pos_ = 0;
  size_t size_;
};

class InputBuffer : public RandomAccessBuffer {
public:
  InputBuffer(size_t size)
    : RandomAccessBuffer(size) {
  }

  size_t read(void* ptr, size_t size) override {
    if (pos_ >= size_) {
      return 0;
    }
    size = std::min(size, size_ - pos_);
    if (size) {
      get_file_contents(ptr, pos_, size);
    }
    pos_ += size;
    return size;
  }

  size_t write(void const* ptr, size_t size) override {
    return 0;
  }

  void truncate() override {}
};

class OutputBuffer : public RandomAccessBuffer {
public:
  OutputBuffer() {
    chunks_.reserve(256);
  }

  int getc() override {
    if (pos_ < size_) {
      auto result = chunks_[pos_ / CHUNK_SIZE][pos_ % CHUNK_SIZE];
      ++pos_;
      return result;
    }
    return EOF;
  }
  void putc(int chr) override {
    size_t chk = pos_ / CHUNK_SIZE;
    if (chk >= chunks_.size()) {
      chunks_.resize(chk + 1);
    }
    chunks_[chk][pos_ % CHUNK_SIZE] = chr;
    ++pos_;
    if (pos_ > size_) {
      size_ = pos_;
    }
  }

  size_t read(void* ptr, size_t size) override {
    uint8_t* dst = (uint8_t*) ptr;
    size_t done = 0;
    while (size && pos_ < size_) {
      size_t chk = pos_ / CHUNK_SIZE;
      size_t offs = pos_ % CHUNK_SIZE;
      size_t have = std::min(size, std::min(size_ - pos_, CHUNK_SIZE - offs));
      memcpy(dst, chunks_[chk].get() + offs, have);
      pos_ += have;
      done += have;
      dst += have;
      size -= have;
    }
    return done;
  }

  size_t write(void const* ptr, size_t size) override {
    if (pos_ + size > size_) {
      size_ = pos_ + size;
      size_t chks = (size_ + CHUNK_SIZE - 1) / CHUNK_SIZE;
      if (chks > chunks_.size()) {
        chunks_.resize(chks);
      }
    }
    size_t done = 0;
    const uint8_t* src = (uint8_t*) ptr;
    while (size) {
      size_t chk = pos_ / CHUNK_SIZE;
      size_t offs = pos_ % CHUNK_SIZE;
      size_t have = std::min(size, CHUNK_SIZE - offs);
      memcpy(chunks_[chk].get() + offs, src, have);
      pos_ += have;
      done += have;
      src += have;
      size -= have;
    }
    return done;
  }

  void truncate() override {
    size_ = pos_;
    size_t chks = (size_ + CHUNK_SIZE - 1) / CHUNK_SIZE;
    if (chks > chunks_.size()) {
      chunks_.resize(chks);
    }
  }

  void flush() {
    put_file_size(size_);
    size_t remain = size_;
    for (size_t i = 0; i < chunks_.size(); ++i) {
      put_file_contents(chunks_[i].get(), i * CHUNK_SIZE, std::min(remain, CHUNK_SIZE));
      remain -= CHUNK_SIZE;
    }
  }

private:
  static const size_t CHUNK_SIZE = 1048576;
  class chunk_t : public std::unique_ptr<uint8_t[]> {
  public:
    chunk_t()
      : std::unique_ptr<uint8_t[]>(std::make_unique<uint8_t[]>(CHUNK_SIZE)) {
      memset(get(), 0, CHUNK_SIZE);
    }
  };
  std::vector<chunk_t> chunks_;
};

class SafeFile : public File {
public:
  SafeFile(const std::shared_ptr<FileBuffer>& buffer)
    : File(buffer)
  {
  }

  size_t safe_read(void* ptr, size_t offset, size_t size) {
#ifdef MULTITHREADING
    std::unique_lock lock(mutex_);
#endif
    seek(offset);
    return read(ptr, size);
  }

private:
  mutex_t mutex_;
};

class MpqChunkedBuffer : public RandomAccessBuffer {
public:
  MpqChunkedBuffer(SafeFile& input, const mpq::MPQBlockEntry& block, uint32_t key, uint32_t blockSize)
    : RandomAccessBuffer(block.fSize)
    , input_(input)
    , entry_(block)
    , key_(key)
    , blockSize_(blockSize)
  {
    if (block.flags & mpq::FileFlags::Compressed) {
      size_t numBlocks = (block.fSize + blockSize - 1) / blockSize;
      size_t blocksSize = (numBlocks + 1) * sizeof(uint32_t);
      blocks_.resize(numBlocks + 1);
      if (input.safe_read(blocks_.data(), block.filePos, blocksSize) != blocksSize) {
        ERROR_MSG("read error");
      }
      if (block.flags & mpq::FileFlags::Encrypted) {
        mpq::decryptBlock(blocks_.data(), blocksSize, key - 1);
      }
    }
  }

  size_t read(void* ptr, size_t size) override {
    if (pos_ >= size_) {
      return 0;
    }
    size = std::min(size, size_ - pos_);

    uint8_t* dst = (uint8_t*) ptr;
    size_t need = size;
    while (need) {
      load(pos_ / blockSize_);
      size_t offset = pos_ % blockSize_;
      size_t have = std::min(need, data_.size() - offset);
      memcpy(dst, data_.data() + offset, have);
      pos_ += have;
      dst += have;
      need -= have;
    }

    return size;
  }

  size_t write(void const* ptr, size_t size) override {
    return 0;
  }

  void truncate() override {}

private:
  SafeFile& input_;
  mpq::MPQBlockEntry entry_;
  uint32_t key_;
  uint32_t blockSize_;
  std::vector<uint32_t> blocks_;
  int current_ = -1;
  std::vector<uint8_t> rawData_;
  std::vector<uint8_t> data_;

  void load(size_t index) {
    if ((int) index == current_) {
      return;
    }
    size_t numBlocks = (size_ + blockSize_ - 1) / blockSize_;
    size_t size = std::min<size_t>(size_ - index * blockSize_, blockSize_);
    data_.resize(size);
    if (entry_.flags & mpq::FileFlags::Compressed) {
      size_t rawSize = blocks_[index + 1] - blocks_[index];
      rawData_.resize(rawSize);
      if (input_.safe_read(rawData_.data(), entry_.filePos + blocks_[index], rawSize) != rawSize) {
        ERROR_MSG("read error");
      }
      if (entry_.flags & mpq::FileFlags::Encrypted) {
        mpq::decryptBlock(rawData_.data(), rawSize, key_ + index);
      }
      uint32_t size1 = size;
      if (entry_.flags & mpq::FileFlags::CompressMulti) {
        if (!mpq::multi_decompress(rawData_.data(), rawSize, data_.data(), &size1) || size1 != size) {
          ERROR_MSG("decompress error");
        }
      } else if (entry_.flags & mpq::FileFlags::CompressPkWare) {
        if (!mpq::pkzip_decompress(rawData_.data(), rawSize, data_.data(), &size1) || size1 != size) {
          ERROR_MSG("decompress error");
        }
      }
    } else {
      if (input_.safe_read(data_.data(), entry_.filePos + index * blockSize_, size) != size) {
        ERROR_MSG("read error");
      }
      if (entry_.flags & mpq::FileFlags::Encrypted) {
        mpq::decryptBlock(data_.data(), size, key_ + index);
      }
    }
    current_ = (int) index;
  }
};

const size_t BLOCK_TYPE = 7;
const size_t BLOCK_SIZE = 1 << (9 + BLOCK_TYPE);

size_t write_mpq(File dst, File src) {
  uint8_t BLOCK_BUF[BLOCK_SIZE];
  uint8_t CMP_BUF[BLOCK_SIZE];

  size_t offset = (size_t) dst.tell();

  size_t num_blocks = ((size_t) src.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
  uint32_t size = (num_blocks + 1) * sizeof(uint32_t);
  std::vector<uint32_t> blockPos(num_blocks + 1);
  dst.seek(size, SEEK_CUR);
  blockPos[0] = size;
  for (size_t i = 0; i < num_blocks; ++i) {
    uint32_t count = (uint32_t) src.read(BLOCK_BUF, BLOCK_SIZE);
    uint32_t comps = sizeof CMP_BUF;
    if (!mpq::multi_compress(BLOCK_BUF, count, CMP_BUF, &comps, 0x02, nullptr)) {
      ERROR_MSG("compression error");
    }
    //if (!mpq::multi_decompress(CMP_BUF, comps, BLOCK_BUF, &count, nullptr)) {
    //  ERROR_MSG("decompression error");
    //}
    dst.write(CMP_BUF, comps);
    blockPos[i + 1] = blockPos[i] + comps;
    size += comps;
  }
  dst.seek(offset, SEEK_SET);
  dst.write(blockPos.data(), blockPos.size() * sizeof(uint32_t));
  dst.seek(offset + size, SEEK_SET);

  return size;
}
size_t write_wave(File dst, File src);

class MpqWriter {
public:
  MpqWriter(size_t tableSize)
    : buffer_(std::make_shared<OutputBuffer>())
    , file_(buffer_)
  {
    header_.id = mpq::MPQHeader::signature;
    header_.headerSize = mpq::MPQHeader::size_v1;
    header_.archiveSize = 0;
    header_.formatVersion = 1;
    header_.blockSize = (uint16_t) BLOCK_TYPE;
    header_.hashTablePos = header_.headerSize;
    header_.blockTablePos = header_.hashTablePos + tableSize * sizeof(mpq::MPQHashEntry);
    header_.hashTableSize = tableSize;
    header_.blockTableSize = tableSize;
    hashTable_.resize(tableSize);
    blockTable_.resize(tableSize);
    memset(blockTable_.data(), 0, sizeof(mpq::MPQBlockEntry) * tableSize);
    memset(hashTable_.data(), -1, sizeof(mpq::MPQHashEntry) * tableSize);
    file_.seek(header_.blockTablePos + tableSize * sizeof(mpq::MPQBlockEntry));
  }

  mpq::MPQBlockEntry& alloc_file(const char* name) {
    size_t index = mpq::hashString(name, mpq::HASH_OFFSET) % hashTable_.size();
    while (hashTable_[index].blockIndex != mpq::MPQHashEntry::EMPTY && hashTable_[index].blockIndex != mpq::MPQHashEntry::DELETED) {
      index = (index + 1) % hashTable_.size();
    }

    hashTable_[index].name1 = mpq::hashString(name, mpq::HASH_NAME1);
    hashTable_[index].name2 = mpq::hashString(name, mpq::HASH_NAME2);
    hashTable_[index].locale = 0;
    hashTable_[index].platform = 0;
    hashTable_[index].blockIndex = blockPos_;

    return blockTable_[blockPos_++];
  }

  void write(const char* name, File src) {
#ifdef MULTITHREADING
    MemoryFile temp;
    size_t usize;
    bool wave = !strcmp(mpq::path_ext(name), ".wav");
    if (wave) {
      usize = write_wave(temp, src);
    } else {
      write_mpq(temp, src);
      usize = (size_t) src.size();
    }
    temp.seek(0);

    std::unique_lock lock(mutex_);
    auto& block = alloc_file(name);
    block.filePos = (uint32_t) file_.tell();
    block.flags = mpq::FileFlags::Exists | (wave ? 0 : mpq::FileFlags::CompressMulti);
    block.cSize = (uint32_t) temp.size();
    block.fSize = usize;
    file_.copy(temp);
#else
    auto& block = alloc_file(name);
    block.filePos = (uint32_t) file_.tell();
    if (strcmp(mpq::path_ext(name), ".wav")) {
      block.flags = mpq::FileFlags::Exists | mpq::FileFlags::CompressMulti;
      block.cSize = write_mpq(file_, src);
      block.fSize = (uint32_t) src.size();
    } else {
      block.flags = mpq::FileFlags::Exists;
      block.cSize = write_wave(file_, src);
      block.fSize = block.cSize;
    }
#endif
  }

  void flush() {
    mpq::encryptBlock(hashTable_.data(), hashTable_.size() * sizeof(mpq::MPQHashEntry), mpq::hashString("(hash table)", mpq::HASH_KEY));
    mpq::encryptBlock(blockTable_.data(), blockTable_.size() * sizeof(mpq::MPQBlockEntry), mpq::hashString("(block table)", mpq::HASH_KEY));

    header_.archiveSize = (uint32_t) file_.size();
    file_.seek(0);
    file_.write(&header_, header_.headerSize);
    file_.write(hashTable_.data(), hashTable_.size() * sizeof(mpq::MPQHashEntry));
    file_.write(blockTable_.data(), blockTable_.size() * sizeof(mpq::MPQBlockEntry));
    file_.seek(0);

    buffer_->flush();
  }

private:
  std::shared_ptr<OutputBuffer> buffer_;
  mutex_t mutex_;
  File file_;
  mpq::MPQHeader header_;
  std::vector<mpq::MPQHashEntry> hashTable_;
  std::vector<mpq::MPQBlockEntry> blockTable_;
  size_t blockPos_ = 0;
  size_t filePos_ = 0;

};

class MpqReader {
public:
  MpqReader(size_t size)
    : file_(std::make_shared<InputBuffer>(size))
  {
    mpq::MPQHeader header;
    if (file_.read(&header, mpq::MPQHeader::size_v1) != mpq::MPQHeader::size_v1 || header.id != mpq::MPQHeader::signature) {
      ERROR_MSG("corrupted MPQ file");
    }
    blockSize_ = (1 << (9 + header.blockSize));
    hashTable_.resize(header.hashTableSize);
    blockTable_.resize(header.blockTableSize);
    readTable(hashTable_.data(), header.hashTablePos, header.hashTableSize * sizeof(mpq::MPQHashEntry), "(hash table)");
    readTable(blockTable_.data(), header.blockTablePos, header.blockTableSize * sizeof(mpq::MPQBlockEntry), "(block table)");
  }

  File read(const char* name) {
    uint32_t index = findFile(name);
    if (index == mpq::MPQHashEntry::EMPTY) {
      return File();
    }
    const auto& block = blockTable_[index];

    uint32_t key = mpq::hashString(mpq::path_name(name), mpq::HASH_KEY);
    if (block.flags & mpq::FileFlags::FixSeed) {
      key = (key + uint32_t(block.filePos)) ^ block.fSize;
    }

    if (block.flags & mpq::FileFlags::SingleUnit) {
      uint32_t csize = block.cSize;
      if (!(block.flags & mpq::FileFlags::Compressed)) {
        csize = block.fSize;
      }
      std::vector<uint8_t> data(csize);
      if (file_.safe_read(data.data(), block.filePos, csize) != csize) {
        ERROR_MSG("read error");
      }
      if (block.flags & mpq::FileFlags::Encrypted) {
        mpq::decryptBlock(data.data(), csize, key);
      }
      if (block.flags & mpq::FileFlags::Compressed) {
        std::vector<uint8_t> temp(block.fSize);
        uint32_t size = block.fSize;
        if (block.flags & mpq::FileFlags::CompressMulti) {
          if (!mpq::multi_decompress(data.data(), csize, temp.data(), &size) || size != block.fSize) {
            ERROR_MSG("decompress error");
          }
        } else if (block.flags & mpq::FileFlags::CompressPkWare) {
          if (!mpq::pkzip_decompress(data.data(), csize, temp.data(), &size) || size != block.fSize) {
            ERROR_MSG("decompress error");
          }
        }
        data.swap(temp);
      }
      return MemoryFile(std::move(data));
    } else {
      return File(std::make_shared<MpqChunkedBuffer>(file_, block, key, blockSize_));
    }
  }

  uint32_t tableSize() const {
    return hashTable_.size();
  }

private:
  SafeFile file_;
  size_t blockSize_;
  std::vector<mpq::MPQHashEntry> hashTable_;
  std::vector<mpq::MPQBlockEntry> blockTable_;

  uint32_t findFile(const char* name) {
    size_t index = mpq::hashString(name, mpq::HASH_OFFSET) % hashTable_.size();
    uint32_t name1 = mpq::hashString(name, mpq::HASH_NAME1);
    uint32_t name2 = mpq::hashString(name, mpq::HASH_NAME2);
    for (size_t i = index, count = 0; hashTable_[i].blockIndex != mpq::MPQHashEntry::EMPTY && count < hashTable_.size(); i = (i + 1) % hashTable_.size(), ++count) {
      if (hashTable_[i].name1 == name1 && hashTable_[i].name2 == name2 && hashTable_[i].blockIndex != mpq::MPQHashEntry::DELETED) {
        return hashTable_[i].blockIndex;
      }
    }
    return mpq::MPQHashEntry::EMPTY;
  }

  void readTable(void* ptr, size_t offset, size_t size, const char* key) {
    file_.seek(offset);
    if (file_.read(ptr, size) != size) {
      ERROR_MSG("corrupted MPQ file");
    }
    mpq::decryptBlock(ptr, size, mpq::hashString(key, mpq::HASH_KEY));
  }
};

const int NumFiles = 2908;
extern const char* ListFile[NumFiles];

#ifdef EMSCRIPTEN
EMSCRIPTEN_KEEPALIVE
extern "C" void DApi_MpqCmp(size_t size) {
#else
int main() {
  __input = File("C:\\Projects\\diabloweb\\run\\DIABDAT0.MPQ", "rb");
  __output = File("C:\\Projects\\diabloweb\\run\\DIABDAT1.MPQ", "wb");
  size_t size = (size_t) __input.size();
#endif

  MpqReader reader(size);
  MpqWriter writer(reader.tableSize());

  int count = 0;

#ifdef MULTITHREADING
  std::atomic<int> next = 0;
  std::vector<std::thread> threads;
  std::mutex mutex;
  for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&next, &reader, &writer, &mutex, &count]() {
      int index;
      while ((index = next++) < NumFiles) {
        auto file = reader.read(ListFile[index]);
        if (file) {
          writer.write(ListFile[index], file);
        }

        std::unique_lock lock(mutex);
        do_progress(++count, NumFiles);
      }
    });
  }
  for (auto& th : threads) {
    th.join();
  }
#else
  for (auto name : ListFile) {
    auto file = reader.read(name);
    if (file) {
      writer.write(name, file);
    }
    do_progress(++count, NumFiles);
  }
#endif
  writer.flush();

#ifndef EMSCRIPTEN
  return 0;
#endif
}
