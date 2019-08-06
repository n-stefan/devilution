#include "common.h"
#include "huffman/huff.h"
#include "pklib/pklib.h"
#include "adpcm/adpcm.h"
#include "../zlib/zlib.h"
#include <algorithm>

namespace mpq {

void* gzalloc(void* opaque, unsigned int items, unsigned int size) {
  return malloc(items * size);
}
void gzfree(void* opaque, void* address) {
  free(address);
}

bool zlib_compress(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  z_stream z;
  memset(&z, 0, sizeof z);
  z.next_in = (Bytef*) in;
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = (Bytef*) out;
  z.avail_out = *out_size;
  z.total_out = 0;
  z.zalloc = gzalloc;
  z.zfree = gzfree;

  int result = deflateInit2(&z, 6, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
  if (result == Z_OK) {
    result = deflate(&z, Z_FINISH);
    *out_size = z.total_out;
    deflateEnd(&z);
  }
  return (result == Z_OK || result == Z_STREAM_END);
}

bool zlib_decompress(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  z_stream z;
  memset(&z, 0, sizeof z);
  z.next_in = (Bytef*) in;
  z.avail_in = in_size;
  z.total_in = in_size;
  z.next_out = (Bytef*) out;
  z.avail_out = *out_size;
  z.total_out = 0;
  z.zalloc = gzalloc;
  z.zfree = gzfree;

  memset(out, 0, *out_size);
  int result = inflateInit(&z);
  if (result == Z_OK) {
    result = inflate(&z, Z_FINISH);
    *out_size = z.total_out;
    inflateEnd(&z);
  }
  return (z.avail_out == 0);
}

struct BUFFERINFO {
  void* out;
  uint32_t outPos;
  uint32_t outLen;
  void* in;
  uint32_t inPos;
  uint32_t inLen;

  static unsigned int input(char* buffer, unsigned int* size, void* param) {
    BUFFERINFO* bi = (BUFFERINFO*)param;
    size_t bufSize = std::min<size_t>(*size, bi->inLen - bi->inPos);
    memcpy(buffer, (char*)bi->in + bi->inPos, bufSize);
    bi->inPos += bufSize;
    return bufSize;
  }

  static void output(char* buffer, unsigned int* size, void* param) {
    BUFFERINFO* bi = (BUFFERINFO*)param;
    size_t bufSize = std::min<size_t>(*size, bi->outLen - bi->outPos);
    memcpy((char*)bi->out + bi->outPos, buffer, bufSize);
    bi->outPos += bufSize;
  }
};

bool pkzip_compress(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  char buf[CMP_BUFFER_SIZE];

  BUFFERINFO bi;
  bi.in = in;
  bi.inPos = 0;
  bi.inLen = in_size;
  bi.out = out;
  bi.outPos = 0;
  bi.outLen = *out_size;

  unsigned int compType = CMP_BINARY;
  unsigned int dictSize;
  if (in_size >= 0xC00) {
    dictSize = 0x1000;
  } else if (in_size < 0x600) {
    dictSize = 0x400;
  } else {
    dictSize = 0x800;
  }

  implode(BUFFERINFO::input, BUFFERINFO::output, buf, &bi, &compType, &dictSize);
  *out_size = bi.outPos;

  return true;
}

bool pkzip_decompress(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  if (*out_size == in_size) {
    memcpy(out, in, in_size);
    return true;
  }
  char buf[EXP_BUFFER_SIZE];

  BUFFERINFO bi;
  bi.in = in;
  bi.inPos = 0;
  bi.inLen = in_size;
  bi.out = out;
  bi.outPos = 0;
  bi.outLen = *out_size;

  unsigned int compType = CMP_BINARY;
  unsigned int dictSize;
  if (in_size >= 0xC00) {
    dictSize = 0x1000;
  } else if (in_size < 0x600) {
    dictSize = 0x400;
  } else {
    dictSize = 0x800;
  }

  explode(BUFFERINFO::input, BUFFERINFO::output, buf, &bi);
  *out_size = bi.outPos;

  return true;
}

bool wave_compress_mono(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  *out_size = CompressADPCM(out, *out_size, in, in_size, 1, 5);
  return true;
}

bool wave_decompress_mono(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  *out_size = DecompressADPCM(out, *out_size, in, in_size, 1);
  return true;
}

bool wave_compress_stereo(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  *out_size = CompressADPCM(out, *out_size, in, in_size, 2, 5);
  return true;
}

bool wave_decompress_stereo(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  *out_size = DecompressADPCM(out, *out_size, in, in_size, 2);
  return true;
}

bool huff_compress(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  THuffmannTree ht(true);
  TOutputStream os(out, *out_size);
  *out_size = ht.Compress(&os, in, in_size, 7);
  return true;
}

bool huff_decompress(void* in, uint32_t in_size, void* out, uint32_t* out_size) {
  THuffmannTree ht(false);
  TInputStream is(in, in_size);
  *out_size = ht.Decompress(out, *out_size, &is);
  return true;
}

struct CompressionType {
  uint8_t id;
  bool(*func) (void* in, uint32_t in_size, void* out, uint32_t* out_size);
};
static CompressionType decomp_table[] = {
  //  {0x10, bzip2_decompress},
  { 0x08, pkzip_decompress },
  { 0x02, zlib_decompress },
  { 0x01, huff_decompress },
  { 0x80, wave_decompress_stereo },
  { 0x40, wave_decompress_mono }
};

bool multi_decompress(void* in, uint32_t in_size, void* out, uint32_t* out_size, void* temp) {
  if (in_size == *out_size) {
    if (in != out) {
      memcpy(out, in, in_size);
    }
    return true;
  }

  uint8_t method = *(uint8_t*)in;
  void* in_ptr = (uint8_t*)in + 1;
  in_size--;

  uint8_t cur_method = method;
  size_t func_count = 0;
  for (auto& met : decomp_table) {
    if ((cur_method & met.id) == met.id) {
      func_count++;
      cur_method &= ~met.id;
    }
  }
  if (cur_method != 0) {
    return false;
  }

  if (func_count == 0) {
    if (in_size > *out_size) {
      return false;
    }
    if (in != out) {
      memcpy(out, in_ptr, in_size);
    }
    *out_size = in_size;
    return true;
  }

  std::vector<uint8_t> temp2;
  void* tmp = temp;
  if (!tmp) {
    temp2.resize(*out_size);
    tmp = temp2.data();
  }

  uint32_t cur_size = in_size;
  void* cur_ptr = in_ptr;
  void* cur_out = ((func_count & 1) && (out != in) ? out : tmp);
  cur_method = method;
  for (auto& met : decomp_table) {
    if ((cur_method & met.id) == met.id) {
      uint32_t size = *out_size;
      if (!met.func(cur_ptr, cur_size, cur_out, &size)) {
        return false;
      }
      cur_size = size;
      cur_ptr = cur_out;
      cur_out = (cur_out == tmp ? out : tmp);
      cur_method &= ~met.id;
    }
  }
  if (cur_ptr != out) {
    memcpy(out, cur_ptr, cur_size);
  }
  *out_size = cur_size;
  return true;
}

}
