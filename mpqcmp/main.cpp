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

#ifdef EMSCRIPTEN

#include <emscripten.h>

EM_JS(void, get_file_contents, (void* ptr, size_t offset, size_t size), {
  self.DApi.get_file_contents(HEAPU8.subarray(ptr, ptr + size), offset);
});

EM_JS(void, put_file_size, (size_t size), {
  self.DApi.put_file_size(size);
});

EM_JS(void, put_file_contents, (void* ptr, size_t offset, size_t size), {
  self.DApi.put_file_contents(HEAPU8.subarray(ptr, ptr + size), offset);
});

EM_JS(void, do_error, (const char* err), {
  var end = HEAPU8.indexOf(0, err);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(err, end));
  self.DApi.exit_error(text);
});
EM_JS(void, do_progress, (int done, int total), {
  self.DApi.progress(done, total);
});

#else

#include <mutex>
#include <thread>

File __input;
std::mutex __mutex;
thread_local File __output;

void get_file_contents(void* ptr, size_t offset, size_t size) {
  std::unique_lock lock(__mutex);
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

std::unordered_map<std::thread::id, std::pair<int, int>> __progress;

void do_error(const char* err) {
  printf("%s\r\n", err);
  exit(0);
}
void do_progress(int done, int total) {
  std::unique_lock lock(__mutex);
  __progress[std::this_thread::get_id()] = std::make_pair(done, total);
  int udone = 0, utotal = 0;
  for (auto& p : __progress) {
    udone += p.second.first;
    utotal += p.second.second;
  }
  printf("\r%d / %d", udone, utotal);
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

void ErrMsg(const char* text, const char *log_file_path, int log_line_nr) {
  do_error(text);
}

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
    : RandomAccessBuffer(size)
  {
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

class MpqChunkedBuffer : public RandomAccessBuffer {
public:
  MpqChunkedBuffer(File input, const mpq::MPQBlockEntry& block, uint32_t key, uint32_t blockSize)
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
      input.seek(block.filePos);
      if (input.read(blocks_.data(), blocksSize) != blocksSize) {
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
  File input_;
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
      input_.seek(entry_.filePos + blocks_[index]);
      if (input_.read(rawData_.data(), rawSize) != rawSize) {
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
      input_.seek(entry_.filePos + index * blockSize_);
      if (input_.read(data_.data(), size) != size) {
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
  File file_;
  mpq::MPQHeader header_;
  std::vector<mpq::MPQHashEntry> hashTable_;
  std::vector<mpq::MPQBlockEntry> blockTable_;
  size_t blockPos_ = 0;
  size_t filePos_ = 0;

};

struct MpqBlock : public mpq::MPQBlockEntry {
  MpqBlock() {}
  MpqBlock(const mpq::MPQBlockEntry& block, uint32_t key, int mode)
    : mpq::MPQBlockEntry(block)
    , key(key)
    , mode(mode)
  {
  }
  uint32_t key;
  int mode;
};

class MpqReader {
public:
  MpqReader(size_t size, uint32_t blockSize)
    : file_(std::make_shared<InputBuffer>(size))
    , blockSize_(blockSize)
  {
  }

  File read(MpqBlock& block) {
    uint32_t key = block.key;
    if (block.flags & mpq::FileFlags::FixSeed) {
      key = (key + block.filePos) ^ block.fSize;
    }

    if (block.flags & mpq::FileFlags::SingleUnit) {
      uint32_t csize = block.cSize;
      if (!(block.flags & mpq::FileFlags::Compressed)) {
        csize = block.fSize;
      }
      std::vector<uint8_t> data(csize);
      file_.seek(block.filePos);
      if (file_.read(data.data(), csize) != csize) {
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

private:
  File file_;
  uint32_t blockSize_;
};

#ifdef EMSCRIPTEN
EMSCRIPTEN_KEEPALIVE
extern "C" void* DApi_Alloc(size_t size) {
  return malloc(size);
}

EMSCRIPTEN_KEEPALIVE
#endif
extern "C" mpq::MPQBlockEntry* DApi_Compress(size_t size, uint32_t blockSize, size_t files, MpqBlock* blocks) {
  MpqReader reader(size, blockSize);

  auto buffer = std::make_shared<OutputBuffer>();
  File output(buffer);

  mpq::MPQBlockEntry* result = new mpq::MPQBlockEntry[files];

  for (size_t i = 0; i < files; ++i) {
    File input = reader.read(blocks[i]);

    auto& block = result[i];
    block.filePos = (uint32_t) output.tell();
    if (blocks[i].mode == 0) {
      block.flags = mpq::FileFlags::Exists | mpq::FileFlags::CompressMulti;
      block.cSize = write_mpq(output, input);
      block.fSize = (uint32_t) input.size();
    } else {
      block.flags = mpq::FileFlags::Exists;
      block.cSize = write_wave(output, input);
      block.fSize = block.cSize;
    }

    do_progress(i + 1, files);
  }

  buffer->flush();

  return result;
}

#ifndef EMSCRIPTEN

const int NumFiles = 2908;
extern const char* ListFile[NumFiles];

template<class T>
std::vector<T> readTable(size_t offset, size_t size, const char* key) {
  std::vector<T> result(size);
  __input.seek(offset);
  if (__input.read(result.data(), size * sizeof(T)) != size * sizeof(T)) {
    ERROR_MSG("corrupted MPQ file");
  }
  mpq::decryptBlock(result.data(), size * sizeof(T), mpq::hashString(key, mpq::HASH_KEY));
  return result;
}

int main() {
  __input = File("C:\\Projects\\diabloweb\\run\\DIABDAT0.MPQ", "rb");

  mpq::MPQHeader header;
  if (__input.read(&header, mpq::MPQHeader::size_v1) != mpq::MPQHeader::size_v1 || header.id != mpq::MPQHeader::signature) {
    ERROR_MSG("corrupted MPQ file");
  }
  auto hashTable = readTable<mpq::MPQHashEntry>(header.hashTablePos, header.hashTableSize, "(hash table)");
  auto blockTable = readTable<mpq::MPQBlockEntry>(header.blockTablePos, header.blockTableSize, "(block table)");

  std::unordered_map<uint64_t, int> fileMap;
  for (int i = 0; i < NumFiles; ++i) {
    uint64_t hash = mpq::hashString(ListFile[i], mpq::HASH_NAME1) | (uint64_t(mpq::hashString(ListFile[i], mpq::HASH_NAME2)) << 32);
    fileMap[hash] = i;
  }

  const int NUM_TASKS = 4;
  struct Task {
    std::vector<std::pair<mpq::MPQHashEntry*, MpqBlock>> input;
    std::pair<MemoryFile, mpq::MPQBlockEntry*> output;
  };
  Task tasks[NUM_TASKS];

  for (auto& entry : hashTable) {
    if (entry.blockIndex == mpq::MPQHashEntry::EMPTY || entry.blockIndex == mpq::MPQHashEntry::DELETED) {
      continue;
    }
    auto it = fileMap.find(entry.name1 | (uint64_t(entry.name2) << 32));
    if (it == fileMap.end()) {
      entry.blockIndex = mpq::MPQHashEntry::DELETED;
      continue;
    }
    auto& block = blockTable[entry.blockIndex];
    int task = block.filePos * NUM_TASKS / (uint32_t) __input.size();
    uint32_t key = mpq::hashString(mpq::path_name(ListFile[it->second]), mpq::HASH_KEY);
    int mode = strcmp(mpq::path_ext(ListFile[it->second]), ".wav") ? 0 : 1;
    tasks[task].input.emplace_back(&entry, MpqBlock(block, key, mode));
  }

  std::vector<mpq::MPQBlockEntry> resBlocks;
  File result("C:\\Projects\\diabloweb\\run\\DIABDAT1.MPQ", "wb");
  size_t hashPos = mpq::MPQHeader::size_v1;
  size_t blockPos = hashPos + hashTable.size() * sizeof(mpq::MPQHashEntry);
  size_t filePos = blockPos + blockTable.size() * sizeof(mpq::MPQBlockEntry);
  uint32_t blockSize = 1 << (9 + header.blockSize);

  std::vector<std::thread> threads;
  std::pair<mpq::MPQBlockEntry*, MemoryFile> thOutput[NUM_TASKS];

  for (auto& task : tasks) {
    threads.emplace_back([blockSize, &task]() mutable {
      std::vector<MpqBlock> data;
      data.reserve(task.input.size());
      for (auto& p : task.input) {
        data.emplace_back(p.second);
      }
      __output = task.output.first = MemoryFile();
      task.output.second = DApi_Compress((size_t) __input.size(), blockSize, task.input.size(), data.data());
    });
  }
  for (auto& th : threads) {
    th.join();
  }

  for (auto& task : tasks) {
    for (size_t i = 0; i < task.input.size(); ++i) {
      task.input[i].first->blockIndex = resBlocks.size();
      task.output.second[i].filePos += filePos;
      resBlocks.push_back(task.output.second[i]);
    }
    result.seek(filePos);
    task.output.first.seek(0);
    result.copy(task.output.first);
    filePos += (size_t) task.output.first.size();
    delete[] task.output.second;
  }

  resBlocks.resize(blockTable.size());
  header.headerSize = mpq::MPQHeader::size_v1;
  header.archiveSize = filePos;
  header.formatVersion = 1;
  header.blockSize = (uint16_t) BLOCK_TYPE;
  header.hashTablePos = hashPos;
  header.blockTablePos = blockPos;

  mpq::encryptBlock(hashTable.data(), hashTable.size() * sizeof(mpq::MPQHashEntry), mpq::hashString("(hash table)", mpq::HASH_KEY));
  mpq::encryptBlock(resBlocks.data(), resBlocks.size() * sizeof(mpq::MPQBlockEntry), mpq::hashString("(block table)", mpq::HASH_KEY));

  result.seek(0);
  result.write(&header, header.headerSize);
  result.write(hashTable.data(), hashTable.size() * sizeof(mpq::MPQHashEntry));
  result.write(resBlocks.data(), resBlocks.size() * sizeof(mpq::MPQBlockEntry));

  return 0;
}
#endif