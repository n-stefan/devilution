#ifndef EMSCRIPTEN
#define NOMINMAX
#include <dsound.h>
#endif
#include "diablo.h"
#include <stdint.h>
#include <vector>
#include "trace.h"
#include "rmpq/file.h"
#include "rmpq/common.h"
#include "rmpq/archive.h"
#include "storm/storm.h"

#ifdef EMSCRIPTEN

#include <emscripten.h>

EM_JS(void, api_create_sound_float, (int id, const float* ptr, int samples, int channels, int rate), {
  self.DApi.create_sound_raw(id, HEAPF32.slice(ptr / 4, ptr / 4 + samples * channels), samples, channels, rate);
});

EM_JS(void, api_create_sound, (int id, const uint8_t* ptr, size_t size), {
  self.DApi.create_sound(id, HEAPU8.slice(ptr, ptr + size));
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

void api_create_sound_raw(int id, const uint8_t* ptr, int samples, int channels, int depth, int rate) {
  std::vector<float> data(samples * channels);
  float* raw = data.data();
  if (depth == 8) {
    for (size_t i = 0; i < samples; ++i) {
      for (int j = 0; j < channels; ++j) {
        raw[samples * j + i] = (float) *ptr++ / 127.5f - 1.0f;
      }
    }
  } else if (depth == 16) {
    const int16_t* i16 = (int16_t*) ptr;
    for (size_t i = 0; i < samples; ++i) {
      for (int j = 0; j < channels; ++j) {
        raw[samples * j + i] = (float) *i16++ / 32768.0f;
      }
    }
  } else {
    return;
  }
  api_create_sound_float(id, raw, samples, channels, rate);
}

#else

#include <unordered_map>
std::unordered_map<int, LPDIRECTSOUNDBUFFER> soundBuffers;

LPDIRECTSOUND sglpDS = NULL;

void api_create_sound_raw(int id, const uint8_t* ptr, int samples, int channels, int depth, int rate) {
  if (!sglpDS) {
    return;
  }

  WAVEFORMATEX fmt;
  fmt.cbSize = 0;
  fmt.wFormatTag = WAVE_FORMAT_PCM;
  fmt.nChannels = channels;
  fmt.nSamplesPerSec = rate;
  fmt.nAvgBytesPerSec = rate * (channels * depth / 8);
  fmt.nBlockAlign = channels * depth / 8;
  fmt.wBitsPerSample = depth;

  size_t size = samples * channels * (depth / 8);

  DSBUFFERDESC desc;
  memset(&desc, 0, sizeof(DSBUFFERDESC));
  desc.dwBufferBytes = size;
  desc.lpwfxFormat = &fmt;
  desc.dwSize = sizeof(DSBUFFERDESC);
  desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_STATIC;

  LPDIRECTSOUNDBUFFER DSB;
  if (sglpDS->CreateSoundBuffer(&desc, &DSB, NULL) == ERROR_SUCCESS) {
    LPVOID buf1, buf2;
    DWORD size1, size2;
    if (DSB->Lock(0, size, &buf1, &size1, &buf2, &size2, 0) == DS_OK) {
      memcpy(buf1, ptr, size);
      DSB->Unlock(buf1, size1, buf2, size2);
    }
    soundBuffers[id] = DSB;
  }
}

void api_create_sound(int id, const uint8_t* ptr, size_t size) {
}

void api_duplicate_sound(int id, int srcId) {
  auto it = soundBuffers.find(srcId);
  if (sglpDS && it != soundBuffers.end()) {
    LPDIRECTSOUNDBUFFER DSB;
    if (sglpDS->DuplicateSoundBuffer(it->second, &DSB) == DS_OK) {
      soundBuffers[id] = DSB;
    }
  }
}

void api_play_sound(int id, int volume, int pan, int loop) {
  auto it = soundBuffers.find(id);
  if (sglpDS && it != soundBuffers.end()) {
    it->second->SetVolume(volume);
    it->second->SetPan(pan);

    it->second->Play(0, 0, loop ? DSBPLAY_LOOPING : 0);
  }
}

void api_set_volume(int id, int volume) {
  auto it = soundBuffers.find(id);
  if (sglpDS && it != soundBuffers.end()) {
    it->second->SetVolume(volume);
  }
}

void api_stop_sound(int id) {
  auto it = soundBuffers.find(id);
  if (sglpDS && it != soundBuffers.end()) {
    it->second->Stop();
  }
}

void api_delete_sound(int id) {
  auto it = soundBuffers.find(id);
  if (sglpDS && it != soundBuffers.end()) {
    it->second->Stop();
    it->second->Release();
    soundBuffers.erase(it);
  }
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

BOOLEAN gbSndInited = FALSE;

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

size_t get_mp3_duration(const uint8_t* data, size_t size);
size_t get_wave_duration(const uint8_t* data, size_t size);

TSnd *sound_file_load(const char *path) {
#ifdef EMSCRIPTEN
  MemoryFile file = ((mpq::Archive*)diabdat_mpq)->load(path);
  if (!file) {
    return NULL;
  }

  size_t duration = get_wave_duration(file.data(), file.size());
  if (!duration) {
    duration = get_mp3_duration(file.data(), file.size());
  }

  int id = nextSoundId++;
  api_create_sound(id, file.data(), file.size());
  return new TSnd(id, (DWORD) duration);
#else
  HANDLE file;
  if (!WOpenFile(path, &file, FALSE)) {
    return NULL;
  }
  WAVEFORMATEX fmt;
  CKINFO chunk;
  BYTE* wave_file = LoadWaveFile(file, &fmt, &chunk);
  if (!wave_file)
    app_fatal("Invalid sound format on file %s", path);

  TSnd* pSnd = sound_from_buffer(wave_file + chunk.dwOffset, chunk.dwSize, fmt.nChannels, fmt.wBitsPerSample, fmt.nSamplesPerSec);

  mem_free_dbg((void *) wave_file);
  WCloseFile(file);

  return pSnd;
#endif
}

TSnd *sound_from_buffer(const uint8_t* buffer, unsigned long size, int channels, int depth, int rate) {
  int id = nextSoundId++;
  int samples = size / (channels * depth / 8);
  api_create_sound_raw(id, buffer, samples, channels, depth, rate);
  return new TSnd(id, (DWORD) ((int64_t) samples * 1000 / rate));
}

void sound_file_cleanup(TSnd *sound_file) {
  delete sound_file;
}

void snd_init() {
  sound_load_volume("Sound Volume", &sglSoundVolume);
  gbSoundOn = sglSoundVolume > VOLUME_MIN;

  sound_load_volume("Music Volume", &sglMusicVolume);
  gbMusicOn = sglMusicVolume > VOLUME_MIN;

#ifndef EMSCRIPTEN
  HMODULE hDsound_dll = LoadLibrary("dsound.dll");
  if (hDsound_dll == NULL) {
    return;
  }

  auto DirectSoundCreate = (HRESULT(WINAPI *)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN))GetProcAddress(hDsound_dll, "DirectSoundCreate");
  if (DirectSoundCreate == NULL) {
    return;
  }

  if (DirectSoundCreate(NULL, &sglpDS, NULL) != DS_OK) {
    sglpDS = NULL;
    return;
  }

  if (sglpDS->SetCooperativeLevel(ghMainWnd, DSSCL_EXCLUSIVE) != DS_OK) {
    sglpDS = NULL;
    return;
  }
#endif
  gbSndInited = TRUE;
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
  if (gbMusicOn && nTrack != NUM_MUSIC) {
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

void sound_store_volume(const char *key, int value) {
  SRegSaveValue("Diablo", key, 0, value);
}

void sound_load_volume(const char *value_name, int *value) {
  int v = *value;
  if (!SRegLoadValue("Diablo", value_name, 0, &v)) {
    v = VOLUME_MAX;
  }
  *value = v;

  if (*value < VOLUME_MIN) {
    *value = VOLUME_MIN;
  } else if (*value > VOLUME_MAX) {
    *value = VOLUME_MAX;
  }
  *value -= *value % 100;
}
