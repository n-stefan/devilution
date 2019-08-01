#include "common.h"
#include <vector>

namespace mpq {

namespace {

uint32_t* cryptTable() {
  static uint32_t table[HASH_SIZE];
  static bool initialized = false;
  if (!initialized) {
    uint32_t seed = 0x00100001;
    for (int i = 0; i < 256; i++) {
      for (int j = i; j < HASH_SIZE; j += 256) {
        seed = (seed * 125 + 3) % 0x2AAAAB;
        uint32_t a = (seed & 0xFFFF) << 16;
        seed = (seed * 125 + 3) % 0x2AAAAB;
        uint32_t b = (seed & 0xFFFF);
        table[j] = a | b;
      }
    }
    initialized = true;
  }
  return table;
}

}

uint32_t hashString(char const* str, uint32_t hashType) {
  uint32_t seed1 = 0x7FED7FED;
  uint32_t seed2 = 0xEEEEEEEE;
  uint32_t* table = cryptTable();
  for (int i = 0; str[i]; i++) {
    unsigned char ch = str[i];
    if (ch >= 'a' && ch <= 'z') {
      ch = ch - 'a' + 'A';
    }
    if (ch == '/') {
      ch = '\\';
    }
    seed1 = table[hashType * 256 + ch] ^ (seed1 + seed2);
    seed2 = ch + seed1 + seed2 * 33 + 3;
  }
  return seed1;
}

void encryptBlock(void* ptr, uint32_t size, uint32_t key) {
  uint32_t seed = 0xEEEEEEEE;
  uint32_t* lptr = (uint32_t*)ptr;
  size /= sizeof(uint32_t);
  uint32_t* table = cryptTable();
  for (uint32_t i = 0; i < size; i++) {
    seed += table[HASH_ENCRYPT * 256 + (key & 0xFF)];
    uint32_t orig = lptr[i];
    lptr[i] ^= key + seed;
    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += orig + seed * 32 + 3;
  }
}

void decryptBlock(void* ptr, uint32_t size, uint32_t key) {
  uint32_t seed = 0xEEEEEEEE;
  uint32_t* lptr = (uint32_t*)ptr;
  size /= sizeof(uint32_t);
  uint32_t* table = cryptTable();
  for (uint32_t i = 0; i < size; i++) {
    seed += table[HASH_ENCRYPT * 256 + (key & 0xFF)];
    lptr[i] ^= key + seed;
    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += lptr[i] + seed * 32 + 3;
  }
}

uint32_t detectTableSeed(uint32_t* blocks, uint32_t offset, uint32_t maxSize) {
  uint32_t temp = (blocks[0] ^ offset) - 0xEEEEEEEE;
  uint32_t* table = cryptTable();
  for (uint32_t i = 0; i < 256; i++)
  {
    uint32_t key = temp - table[HASH_ENCRYPT * 256 + i];
    if ((key & 0xFF) != i)
      continue;

    uint32_t seed = 0xEEEEEEEE + table[HASH_ENCRYPT * 256 + (key & 0xFF)];
    if ((blocks[0] ^ (key + seed)) != offset)
      continue; // sanity check

    uint32_t saved = key + 1;
    key = ((~key << 21) + 0x11111111) | (key >> 11);
    seed += offset + seed * 32 + 3;

    seed += table[HASH_ENCRYPT * 256 + (key & 0xFF)];
    if ((blocks[1] ^ (key + seed)) <= offset + maxSize)
      return saved;
  }
  return 0;
}

uint32_t detectFileSeed(uint32_t* data, uint32_t size) {
  uint32_t fileTable[3][3] = {
    { 0x46464952 /*RIFF*/, size - 8, 0x45564157 /*WAVE*/ },
    { 0x00905A4D, 0x00000003 },
    { 0x34E1F3B9, 0xD5B0DBFA },
  };
  uint32_t* table = cryptTable();
  uint32_t tSize[3] = { 3, 2, 2 };
  for (uint32_t set = 0; set < 3; set++) {
    uint32_t temp = (data[0] ^ fileTable[set][0]) - 0xEEEEEEEE;
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t key = temp - table[HASH_ENCRYPT * 256 + i];
      if ((key & 0xFF) != i)
        continue;
      uint32_t seed = 0xEEEEEEEE + table[HASH_ENCRYPT * 256 + (key & 0xFF)];
      if ((data[0] ^ (key + seed)) != fileTable[set][0])
        continue;

      uint32_t saved = key;
      for (uint32_t j = 1; j < tSize[set]; j++) {
        key = ((~key << 21) + 0x11111111) | (key >> 11);
        seed += fileTable[set][j - 1] + seed * 32 + 3;
        seed += table[HASH_ENCRYPT * 256 + (key & 0xFF)];
        if ((data[j] ^ (key + seed)) != fileTable[set][j])
          break;
        if (j == tSize[set] - 1)
          return saved;
      }
    }
  }
  return 0;
}

bool validateHash(void const* buf, uint32_t size, void const* expected) {
  uint32_t sum = ((uint32_t*)expected)[0] | ((uint32_t*)expected)[1] | ((uint32_t*)expected)[2] | ((uint32_t*)expected)[3];
  if (sum == 0) {
    return true;
  }
  uint8_t digest[MD5::DIGEST_SIZE];
  MD5::checksum(buf, size, digest);
  return memcmp(digest, expected, MD5::DIGEST_SIZE) == 0;
}

bool validateHash(File file, void const* expected) {
  uint32_t sum = ((uint32_t*)expected)[0] | ((uint32_t*)expected)[1] | ((uint32_t*)expected)[2] | ((uint32_t*)expected)[3];
  if (sum == 0) {
    return true;
  }
  uint8_t digest[MD5::DIGEST_SIZE];
  file.md5(digest);
  return memcmp(digest, expected, MD5::DIGEST_SIZE) == 0;
}

void MPQHeader::upgrade(size_t fileSize) {
  if (formatVersion <= 1) {
    formatVersion = 1;
    headerSize = size_v1;
    archiveSize = fileSize;
    hiBlockTablePos64 = 0;
    hashTablePosHi = 0;
    blockTablePosHi = 0;
  }
  if (formatVersion <= 3) {
    if (headerSize < size_v3) {
      archiveSize64 = archiveSize;
      if (fileSize > std::numeric_limits<uint32_t>::max()) {
        uint64_t pos = offset64(hashTablePos, hashTablePosHi) + hashTableSize * sizeof(MPQHashEntry);
        if (pos > archiveSize64) {
          archiveSize64 = pos;
        }
        pos = offset64(blockTablePos, blockTablePosHi) + blockTableSize * sizeof(MPQBlockEntry);
        if (pos > archiveSize64) {
          archiveSize64 = pos;
        }
        if (hiBlockTablePos64) {
          pos = hiBlockTablePos64 + blockTableSize * sizeof(uint16_t);
          if (pos > archiveSize64) {
            archiveSize64 = pos;
          }
        }
      }

      hetTablePos64 = 0;
      betTablePos64 = 0;
    }

    hetTableSize64 = 0;
    betTableSize64 = 0;
    if (hetTablePos64) {
      hetTableSize64 = betTablePos64 - hetTablePos64;
      betTableSize64 = offset64(hashTablePos, hashTablePosHi) - betTablePos64;
    }

    if (formatVersion >= 2) {
      hashTableSize64 = offset64(blockTablePos, blockTablePosHi) - offset64(hashTablePos, hashTablePosHi);
      if (hiBlockTablePos64) {
        blockTableSize64 = hiBlockTablePos64 - offset64(blockTablePos, blockTablePosHi);
        hiBlockTableSize64 = archiveSize64 - hiBlockTablePos64;
      } else {
        blockTableSize64 = archiveSize64 - offset64(blockTablePos, blockTablePosHi);
        hiBlockTableSize64 = 0;
      }
    } else {
      hashTableSize64 = uint64_t(hashTableSize) * sizeof(MPQHashEntry);
      blockTableSize64 = uint64_t(blockTableSize) * sizeof(MPQBlockEntry);
      hiBlockTableSize64 = 0;
    }

    rawChunkSize = 0;
    memset(md5BlockTable, 0, sizeof md5BlockTable);
    memset(md5HashTable, 0, sizeof md5HashTable);
    memset(md5HiBlockTable, 0, sizeof md5HiBlockTable);
    memset(md5BetTable, 0, sizeof md5BetTable);
    memset(md5HetTable, 0, sizeof md5HetTable);
    memset(md5MPQHeader, 0, sizeof md5MPQHeader);
  }
}

const char* path_name(const char* path) {
  size_t pos = strlen(path);
  while (pos > 0 && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
  }
  return path + pos;
}

const char* path_ext(const char* path) {
  size_t pos = 0;
  while (path[pos]) {
    ++pos;
  }
  size_t dot = pos;
  while (pos && path[pos - 1] != '/' && path[pos - 1] != '\\') {
    --pos;
    if (path[pos] == '.' && !path[dot]) {
      dot = pos;
    }
  }
  if (dot == pos) {
    return "";
  } else {
    return path + dot;
  }
}

}
