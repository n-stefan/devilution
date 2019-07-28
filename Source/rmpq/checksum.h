#pragma once

#include <string>

uint32_t update_crc(uint32_t crc, void const* vbuf, uint32_t length);
uint32_t crc32(void const* buf, uint32_t length);
uint32_t crc32(std::string const& str);

class MD5 {
  uint8_t buffer[64];
  uint32_t digest[4];
  uint64_t length;
  uint32_t bufSize;
  void run();
public:
  enum { DIGEST_SIZE = 16 };

  MD5();
  void process(void const* buf, uint32_t size);
  void finish(void* digest);

  static std::string format(void const* digest);

  static void checksum(void const* buf, uint32_t size, void* digest) {
    MD5 md5;
    md5.process(buf, size);
    md5.finish(digest);
  }
};

uint64_t jenkins(void const* buf, uint32_t length);
uint32_t hashlittle(void const* buf, uint32_t length, uint32_t initval);
