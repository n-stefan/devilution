#include <stdint.h>
#include "diablo.h"

namespace {

size_t skipID3v2Tag(const uint8_t* block) {
  if (block[0] == 'I' && block[1] == 'D' && block[2] == '3') {
    int id3v2_major_version = block[3];
    int id3v2_minor_version = block[4];
    int id3v2_flags = block[5];
    int flag_unsynchronisation = id3v2_flags & 0x80 ? 1 : 0;
    int flag_extended_header = id3v2_flags & 0x40 ? 1 : 0;
    int flag_experimental_ind = id3v2_flags & 0x20 ? 1 : 0;
    int flag_footer_present = id3v2_flags & 0x10 ? 1 : 0;
    int z0 = block[6];
    int z1 = block[7];
    int z2 = block[8];
    int z3 = block[9];
    if (((z0 & 0x80) == 0) && ((z1 & 0x80) == 0) && ((z2 & 0x80) == 0) && ((z3 & 0x80) == 0)) {
      size_t header_size = 10;
      size_t tag_size = ((z0 & 0x7F) << 21) + ((z1 & 0x7F) << 14) + ((z2 & 0x7F) << 7) + (z3 & 0x7F);
      size_t footer_size = flag_footer_present ? 10 : 0;
      return header_size + tag_size + footer_size;
    }
  }
  return 0;
}

size_t framesize(int samples, int layerBits, int bitrate, int sampleRate, int paddingBit) {
  if (layerBits == 3) { // layer 1
    return ((samples * bitrate * 125) / sampleRate) + paddingBit * 4;
  } else {
    return ((samples * bitrate * 125) / sampleRate) + paddingBit;
  }
}

static int bitRatesV1L1[16] = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0};
static int bitRatesV1L2[16] = {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0};
static int bitRatesV1L3[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
static int bitRatesV2L1[16] = {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0};
static int bitRatesV2L2[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};

inline int getBitRate(int version, int layer, int index) {
  if (version == 3) {
    switch (layer) {
    case 0: return 0;
    case 1: return bitRatesV1L3[index];
    case 2: return bitRatesV1L2[index];
    case 3: return bitRatesV1L1[index];
    default: return 0;
    }
  } else if (version == 1) {
    return 0;
  } else {
    switch (layer) {
    case 0: return 0;
    case 1: return bitRatesV2L2[index];
    case 2: return bitRatesV2L2[index];
    case 3: return bitRatesV2L1[index];
    default: return 0;
    }
  }
}
static int sampleRateTbl[16] = {
  11025, 12000, 8000, 0, // 2.5
  0, 0, 0, 0, // x
  22050, 24000, 16000, 0, // 2
  44100, 48000, 32000, 0, // 1
};
inline int getSampleRate(int version, int index) {
  return sampleRateTbl[version * 4 + index];
}
static int samplesTbl[16] = {
  0, 576, 1152, 384, // 2.5
  0, 0, 0, 0,
  0, 576, 1152, 384, // 2
  0, 1152, 1152, 384, // 1
};
inline int getSamples(int version, int layer) {
  return samplesTbl[version * 4 + layer];
}

size_t parseFrameHeader(const uint8_t* header, double& duration) {
  int b1 = header[1];
  int b2 = header[2];
  int versionBits = (b1 & 0x18) >> 3; // 2.5, x, 2, 1
  int layerBits = (b1 & 0x06) >> 1; // x, 3, 2, 1
  int bitRateIndex = (b2 & 0xF0) >> 4;
  int bitrate = getBitRate(versionBits, layerBits, bitRateIndex);
  if (!bitrate) {
    return 1;
  }
  int sampleRateIndex = (b2 & 0x0C) >> 2;
  int sampleRate = getSampleRate(versionBits, sampleRateIndex);
  if (!sampleRate) {
    return 1;
  }
  int samples = getSamples(versionBits, layerBits);
  int paddingBit = (b2 & 0x02) >> 1;

  duration += (double) samples / (double) sampleRate;
  return framesize(samples, layerBits, bitrate, sampleRate, paddingBit);
}

bool read_wave_section(const uint8_t* data, size_t size, size_t& offset, DWORD id, CKINFO& ck) {
  DWORD hdr[2];

  while (true) {
    if (offset + 8 > size) {
      return false;
    }
    memcpy(&hdr, data + offset, 8);
    offset += 8;
    if (hdr[0] == id) {
      break;
    }
    offset += hdr[1];
  }

  ck.dwSize = hdr[1];
  ck.dwOffset = offset;
  return true;
}

}

size_t get_mp3_duration(const uint8_t* data, size_t size) {
  double duration = 0.0;
  if (size < 10) {
    return 0;
  }
  size_t offset = skipID3v2Tag(data);
  while (offset + 10 <= size) {
    if (data[offset] == 0xFF && (data[offset + 1] & 0xE0)) {
      offset += parseFrameHeader(data + offset, duration);
    } else if (data[offset] == 'T' && data[offset + 1] == 'A' && data[offset + 2] == 'G') {
      offset += 128;
    } else {
      offset += 1;
    }
  }
  return size_t(duration * 1000);
}

size_t get_wave_duration(const uint8_t* data, size_t size) {
  MMCKINFO mmck;
  CKINFO fmt;
  PCMWAVEFORMAT wf;

  if (size < 12) {
    return 0;
  }
  memcpy(&mmck, data, 12);
  if (mmck.ckid != FOURCC_RIFF || mmck.fccType != MAKEFOURCC('W', 'A', 'V', 'E')) {
    return 0;
  }

  size_t offset = 12;
  if (!read_wave_section(data, size, offset, MAKEFOURCC('f', 'm', 't', ' '), fmt)) {
    return 0;
  }

  if (fmt.dwSize < sizeof(PCMWAVEFORMAT) || fmt.dwSize + fmt.dwOffset > size) {
    return 0;
  }

  memcpy(&wf, data + fmt.dwOffset, sizeof wf);
  offset += fmt.dwSize;

  if (!read_wave_section(data, size, offset, MAKEFOURCC('d', 'a', 't', 'a'), fmt)) {
    return 0;
  }

  int channels = wf.wf.nChannels;
  int depth = wf.wBitsPerSample;
  int rate = wf.wf.nSamplesPerSec;
  int samples = fmt.dwSize / (channels * depth / 8);
  return (size_t) ((int64_t) samples * 1000 / rate);
}
