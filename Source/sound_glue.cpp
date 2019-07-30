#include "diablo.h"
#include <stdint.h>
#include <vector>
#include "trace.h"
#include "rmpq/file.h"
#include "rmpq/common.h"
#include "storm/storm.h"

#ifdef EMSCRIPTEN

#include <emscripten.h>

EM_JS(void, api_create_sound, (int id, const float* ptr, int samples, int channels, int rate), {
  self.DApi.create_sound(id, HEAPF32.slice(ptr / 4, ptr / 4 + samples * channels), samples, channels, rate);
});

EM_JS(void, api_duplicate_sound, (int id, int srcId), {
  self.DApi.duplicate_sound(id, srcId);
});

EM_JS(void, api_play_sound, (int id, int volume, int pan, int loop), {
  self.DApi.play_sound(id, volume, pan, loop);
});

EM_JS(void, api_set_volume, (int id, int volume), {
  self.DApi.set_volume(id, volume);
});

EM_JS(void, api_stop_sound, (int id), {
  self.DApi.stop_sound(id);
});

EM_JS(void, api_delete_sound, (int id), {
  self.DApi.delete_sound(id);
});

#else

void api_create_sound(int id, const float* buffer, int samples, int channels, int rate) {
}
void api_duplicate_sound(int id, int srcId) {
}
void api_play_sound(int id, int volume, int pan, int loop) {
}
void api_set_volume(int id, int volume) {
}
void api_stop_sound(int id) {
}
void api_delete_sound(int id) {
}

#endif

static int nextSoundId = 0;

struct TSnd {
public:
  TSnd(int id, DWORD duration)
    : id_(id)
    , duration_(duration)
  {
  }
  ~TSnd() {
    api_delete_sound(id_);
  }

  bool playing() {
    return started_ && (looping_ || _GetTickCount() < started_ + duration_);
  }
  bool recently_started() {
    return started_ && _GetTickCount() - started_ < 80;
  }
  void play(int volume, int pan, bool loop = false) {
    started_ = _GetTickCount();
    looping_ = loop;
    api_play_sound(id_, volume, pan, loop ? 1 : 0);
  }
  void stop() {
    started_ = 0;
    looping_ = false;
    api_stop_sound(id_);
  }
  void set_volume(int volume) {
    api_set_volume(id_, volume);
  }

  void reset() {
    started_ = 0;
  }

  TSnd* duplicate() {
    int id = nextSoundId++;
    api_duplicate_sound(id, id_);
    return new TSnd(id, duration_);
  }

private:
  int id_;
  DWORD started_ = 0;
  bool looping_ = false;
  DWORD duration_;
};

TSnd* DSBs[8];
TSnd* sgpMusicTrack = nullptr;

int sglMusicVolume;
int sglSoundVolume;

BOOLEAN gbSndInited = 1;

BOOLEAN gbMusicOn = 1;
BOOLEAN gbSoundOn = 1;
BOOLEAN gbDupSounds = 1;
int sgnMusicTrack = NUM_MUSIC;
const char *sgszMusicTracks[NUM_MUSIC] = {
#ifndef SPAWN
  "Music\\DTowne.wav",
  "Music\\DLvlA.wav",
  "Music\\DLvlB.wav",
  "Music\\DLvlC.wav",
  "Music\\DLvlD.wav",
  "Music\\Dintro.wav"
#else
  "Music\\STowne.wav",
  "Music\\SLvlA.wav",
  "Music\\Sintro.wav"
#endif
};

char unk_volume[4][2] = {
  { 15, -16 },
  { 15, -16 },
  { 30, -31 },
  { 30, -31 }
};

void snd_update(BOOL bStopAll) {
  for (int i = 0; i < 8; i++) {
    if (!DSBs[i])
      continue;

    if (!bStopAll && DSBs[i]->playing())
      continue;

    delete DSBs[i];
    DSBs[i] = nullptr;
  }
}

void snd_stop_snd(TSnd *pSnd) {
  if (pSnd) {
    pSnd->stop();
  }
}

BOOL snd_playing(TSnd *pSnd) {
  return pSnd && pSnd->playing();
}

void sound_reset(TSnd* pSnd) {
  pSnd->reset();
}

TSnd* sound_dup_channel(TSnd* snd) {
  if (!gbDupSounds) {
    return nullptr;
  }

  for (int i = 0; i < 8; i++) {
    if (!DSBs[i]) {
      return DSBs[i] = snd->duplicate();
    }
  }

  return nullptr;
}

void snd_play_snd(TSnd *pSnd, int lVolume, int lPan) {
  if (!pSnd || !gbSoundOn) {
    return;
  }

  if (pSnd->recently_started()) {
    return;
  }

  if (pSnd->playing()) {
    pSnd = sound_dup_channel(pSnd);
    if (!pSnd) {
      return;
    }
  }

  lVolume += sglSoundVolume;
  if (lVolume < VOLUME_MIN) {
    lVolume = VOLUME_MIN;
  } else if (lVolume > VOLUME_MAX) {
    lVolume = VOLUME_MAX;
  }
  pSnd->play(lVolume, lPan);
}

TSnd *sound_file_load(const char *path) {
  //S
  //File ff(path, "rb");
  //File ff2("C:/Projects/")
  HANDLE file;
  if (!WOpenFile(path, &file, FALSE)) {
    return NULL;
  }
  //{
  //  auto size = SFileGetFileSize(file, NULL);
  //  std::vector<BYTE> data(size);
  //  SFileReadFile(file, data.data(), data.size(), NULL, NULL);
  //  SFileSetFilePointer(file, 0, NULL, SEEK_SET);
  //  File ff(std::string("audio/") + mpq::path_name(path), "wb");
  //  ff.write(data.data(), size);
  //}
  WAVEFORMATEX fmt;
  CKINFO chunk;
  BYTE* wave_file = LoadWaveFile(file, &fmt, &chunk);
  if (!wave_file)
    app_fatal("Invalid sound format on file %s", path);

  TSnd* pSnd = sound_from_buffer(wave_file + chunk.dwOffset, chunk.dwSize, fmt.nChannels, fmt.wBitsPerSample, fmt.nSamplesPerSec);

  mem_free_dbg((void *) wave_file);
  WCloseFile(file);

  return pSnd;
}

class BufferReader {
public:
  BufferReader(const unsigned char* ptr)
    : ptr_(ptr)
  {
  }
  float next8() {
    uint8_t value = (uint8_t) *ptr_++;
    return (float) value / 255.0f - 0.5f;
  }
  float next16() {
    int16_t value = *(int16_t*) ptr_;
    ptr_ += 2;
    return (float) value / 32768.0f;
  }
  float next24() {
    int32_t value = int32_t(*(uint16_t*) ptr_) + (uint32_t(ptr_[2]) << 16);
    ptr_ += 3;
    if (value >= 0x800000) {
      value -= 0x1000000;
    }
    return (float) value / 8388608.0f;
  }
  float next32() {
    int32_t value = *(int32_t*) ptr_;
    ptr_ += 4;
    return (float) value / 2147483648.0f;
  }

  typedef float (BufferReader::*Reader)();

  static Reader reader(int depth) {
    switch (depth) {
    case 8:
      return &BufferReader::next8;
    case 16:
      return &BufferReader::next16;
    case 24:
      return &BufferReader::next24;
    case 32:
      return &BufferReader::next32;
    default:
      return nullptr;
    }
  }
private:
  const unsigned char* ptr_;
};

TSnd *sound_from_buffer(const unsigned char* buffer, unsigned long size, int channels, int depth, int rate) {
  BufferReader reader(buffer);
  BufferReader::Reader func = BufferReader::reader(depth);
  if (!func) {
    return nullptr;
  }
  size_t sampleSize = depth / 8;
  size_t numSamples = size / sampleSize;
  size_t numBlocks = numSamples / channels;
  std::vector<float> samples(numSamples);
  float* raw = samples.data();
  for (size_t i = 0; i < numBlocks; ++i) {
    for (int j = 0; j < channels; ++j) {
      raw[numBlocks * j + i] = (reader.*func)();
    }
  }
  int id = nextSoundId++;
  api_create_sound(id, raw, (int) numBlocks, channels, rate);
  return new TSnd(id, (DWORD) ((int64_t) numBlocks * 1000 / rate));
}

void sound_file_cleanup(TSnd *sound_file) {
  delete sound_file;
}

void snd_init(HWND hWnd) {
  sound_load_volume("Sound Volume", &sglSoundVolume);
  gbSoundOn = sglSoundVolume > VOLUME_MIN;

  sound_load_volume("Music Volume", &sglMusicVolume);
  gbMusicOn = sglMusicVolume > VOLUME_MIN;
}

void sound_cleanup() {
  snd_update(TRUE);
}

void music_stop() {
  if (sgpMusicTrack) {
    sound_file_cleanup(sgpMusicTrack);
    sgpMusicTrack = NULL;
    sgnMusicTrack = NUM_MUSIC;
  }
}

void music_start(int nTrack) {
  music_stop();
  if (gbMusicOn) {
    sgpMusicTrack = sound_file_load(sgszMusicTracks[nTrack]);
    sgnMusicTrack = nTrack;
    if (sgpMusicTrack) {
      sgpMusicTrack->play(sglMusicVolume, 0, true);
    }
  }
}

void sound_disable_music(BOOL disable) {
  if (disable) {
    music_stop();
  } else if (sgnMusicTrack != NUM_MUSIC) {
    music_start(sgnMusicTrack);
  }
}

int sound_get_or_set_music_volume(int volume) {
  if (volume == 1)
    return sglMusicVolume;

  sglMusicVolume = volume;

  if (sgpMusicTrack) {
    sgpMusicTrack->set_volume(volume);
  }

  return sglMusicVolume;
}

int sound_get_or_set_sound_volume(int volume) {
  if (volume == 1)
    return sglSoundVolume;

  sglSoundVolume = volume;

  return sglSoundVolume;
}
