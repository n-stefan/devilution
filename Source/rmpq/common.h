#pragma once

#include "file.h"
#include "checksum.h"

namespace mpq {

inline uint64_t offset64(uint32_t offset, uint16_t offsetHi) {
  return uint64_t(offset) | (uint64_t(offsetHi) << 32);
}

inline uint64_t hashTo64(uint32_t name1, uint32_t name2) {
  return uint64_t(name1) | (uint64_t(name2) << 32);
}

enum {
  HASH_OFFSET = 0,
  HASH_NAME1 = 1,
  HASH_NAME2 = 2,
  HASH_KEY = 3,
  HASH_ENCRYPT = 4,
  HASH_SIZE = 1280
};

#pragma pack(push, 1)
struct MPQUserData {
  enum {
    signature = 0x1B51504D, // MPQ\x1B
  };
  uint32_t id;
  uint32_t userDataSize;
  uint32_t headerOffs;
  uint32_t userDataHeader;
};
struct MPQHeader {
  enum {
    signature = 0x1A51504D, // MPQ\x1A
    size_v1 = 32,
    size_v2 = 44,
    size_v3 = 68,
    size_v4 = 208,
  };

  uint32_t id;
  uint32_t headerSize;
  uint32_t archiveSize;
  uint16_t formatVersion;
  uint16_t blockSize;

  uint32_t hashTablePos;
  uint32_t blockTablePos;
  uint32_t hashTableSize;
  uint32_t blockTableSize;

  // v2
  uint64_t hiBlockTablePos64;
  uint16_t hashTablePosHi;
  uint16_t blockTablePosHi;

  // v3
  uint64_t archiveSize64;
  uint64_t betTablePos64;
  uint64_t hetTablePos64;

  // v4
  uint64_t hashTableSize64;
  uint64_t blockTableSize64;
  uint64_t hiBlockTableSize64;
  uint64_t hetTableSize64;
  uint64_t betTableSize64;

  uint32_t rawChunkSize;
  uint8_t md5BlockTable[MD5::DIGEST_SIZE];
  uint8_t md5HashTable[MD5::DIGEST_SIZE];
  uint8_t md5HiBlockTable[MD5::DIGEST_SIZE];
  uint8_t md5BetTable[MD5::DIGEST_SIZE];
  uint8_t md5HetTable[MD5::DIGEST_SIZE];
  uint8_t md5MPQHeader[MD5::DIGEST_SIZE];

  void upgrade(size_t fileSize);
};

struct MPQHashEntry {
  enum {
    EMPTY = 0xFFFFFFFF,
    DELETED = 0xFFFFFFFE,
  };
  uint32_t name1;
  uint32_t name2;
  uint16_t locale;
  uint16_t platform;
  uint32_t blockIndex;
};

struct MPQBlockEntry {
  uint32_t filePos;
  uint32_t cSize;
  uint32_t fSize;
  uint32_t flags;
};
#pragma pack(pop)

uint32_t hashString(char const* str, uint32_t hashType);

inline uint64_t hashString64(char const* str) {
  return hashTo64(hashString(str, HASH_NAME1), hashString(str, HASH_NAME2));
}

void encryptBlock(void* ptr, uint32_t size, uint32_t key);
void decryptBlock(void* ptr, uint32_t size, uint32_t key);

uint32_t detectTableSeed(uint32_t* blocks, uint32_t offset, uint32_t maxSize);
uint32_t detectFileSeed(uint32_t* data, uint32_t size);

bool validateHash(void const* buf, uint32_t size, void const* expected);
bool validateHash(File file, void const* expected);

#define ID_WAVE         0x46464952
bool pkzip_compress(void* in, size_t in_size, void* out, size_t* out_size);
bool pkzip_decompress(void* in, size_t in_size, void* out, size_t* out_size);
//bool multi_compress(void* in, size_t in_size, void* out, size_t* out_size, uint8_t mask, void* temp);
bool multi_decompress(void* in, size_t in_size, void* out, size_t* out_size, void* temp = nullptr);

const char* path_name(const char* path);

}
