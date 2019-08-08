#include "common.h"
#include "dialog.h"
#include <algorithm>

#include "../storm/storm.h"

#include "../libsmacker/smacker.h"

class SmkVideo {
public:
  SmkVideo(const char *name, bool loop) {
    loop_ = loop;
    if (!SFileOpenFile(name, &file_)) {
      return;
    }
    int size = SFileGetFileSize(file_, nullptr);
    buffer_.resize(size);
    if (!SFileReadFile(file_, buffer_.data(), buffer_.size(), nullptr,
                       nullptr)) {
      return;
    }
    video_ = smk_open_memory(buffer_.data(), buffer_.size());
    if (!video_) {
      return;
    }

    unsigned long nFrames;
    smk_info_all(video_, nullptr, &nFrames, &frameLength_);
    smk_info_video(video_, &width_, &height_, nullptr);
    smk_enable_video(video_, true);

    unsigned char channels[7], depth[7];
    unsigned long rate[7];
    smk_info_audio(video_, nullptr, channels, depth, rate);
    if ( depth[0] )
    {
      std::vector<BYTE> audio_buffer;
      smk_enable_audio( video_, 0, true );
      smk_enable_video( video_, false );
      smk_first( video_ );
      do
      {
        auto buffer = smk_get_audio( video_, 0 );
        auto bufferSize = smk_get_audio_size( video_, 0 );
        audio_buffer.insert( audio_buffer.end(), buffer, buffer + bufferSize );
      } while ( smk_next( video_ ) );
      sound_ = sound_from_buffer( audio_buffer.data(), audio_buffer.size(), channels[0], depth[0], (int) rate[0] );
      smk_enable_video( video_, true );
      smk_enable_audio( video_, 0, false );
    }

    smk_first( video_ );
    repaint_ = true;

    loaded_ = true;
  }

  bool loaded() const {
    return loaded_;
  }

  bool render(unsigned int time) {
    double dtime = (double)time * 1000.0;
    if (frameEnd_ < 0) {
      frameEnd_ = dtime + frameLength_;
    }

    if (smk_palette_updated(video_)) {
      auto src = smk_get_palette(video_);
      PALETTEENTRY pal[256];
      for (int i = 0; i < 256; ++i) {
        pal[i].peFlags = 0;
        pal[i].peRed = src[i * 3];
        pal[i].peGreen = src[i * 3 + 1];
        pal[i].peBlue = src[i * 3 + 2];
      }
      set_palette(pal);
    }

    if (sound_ && !snd_playing(sound_)) {
      snd_play_snd( sound_, 0, 0 );
    }

    if (repaint_) {
      repaint_ = false;
      auto frame = smk_get_video(video_);
      int uwidth = width_, uheight = height_, scale = 1;
      if (uwidth * 2 <= SCREEN_WIDTH && uheight * 2 <= SCREEN_HEIGHT) {
        uwidth *= 2;
        uheight *= 2;
        scale *= 2;
      }
      int screenY = (SCREEN_HEIGHT - uheight) / 2;
      int screenX = (SCREEN_WIDTH - uwidth) / 2;
      lock_buf(1);
      for (int i = 0; i < SCREEN_HEIGHT; ++i) {
        BYTE *dst = (BYTE *) gpBuffer + (SCREEN_Y + i) * BUFFER_WIDTH + SCREEN_X;
        if (i < screenY || i >= screenY + uheight) {
          memset(dst, 0, SCREEN_WIDTH);
        } else {
          const BYTE *src = frame + ((i - screenY) / scale) * width_;
          if (screenX > 0) {
            memset(dst, 0, screenX);
          }
          int srcX = 0, dstX = screenX, srcW = uwidth;
          if (dstX < 0) {
            srcX = -dstX;
            dstX = 0;
          }
          if (dstX + srcW > SCREEN_WIDTH) {
            srcW = SCREEN_WIDTH - dstX;
          }
          if (scale == 1) {
            memcpy(dst + dstX, src + srcX, srcW);
          } else {
            srcW /= scale;
            BYTE *dptr = dst + dstX;
            const BYTE *sptr = src + srcX;
            for (int x = 0; x < srcW; ++x) {
              *dptr++ = *sptr;
              *dptr++ = *sptr++;
            }
          }
          if (screenX + uwidth < SCREEN_WIDTH) {
            memset(dst + screenX + uwidth, 0, SCREEN_WIDTH - screenX - uwidth);
          }
        }
      }
      unlock_buf(1);
      if (draw_lock(nullptr)) {
        draw_blit(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        draw_unlock();
        draw_flush();
      }
    }

    while (dtime >= frameEnd_) {
      if (!nextFrame_()) {
        return false;
      }
    }
    return true;
  }

  void stop() {
    if (sound_) {
      snd_stop_snd(sound_);
    }
  }

  ~SmkVideo() {
    if (video_) {
      smk_close(video_);
    }
    if (file_) {
      SFileCloseFile(file_);
    }
    if (sound_) {
      sound_file_cleanup( sound_ );
    }
  }

private:
  HANDLE file_;
  smk video_;
  bool loop_;
  double frameLength_;
  double frameEnd_ = -1;
  unsigned long width_, height_;
  std::vector<BYTE> buffer_;
  TSnd* sound_ = nullptr;
  bool repaint_ = false;

  bool loaded_ = false;

  bool nextFrame_() {
    frameEnd_ += frameLength_;
    if (smk_next(video_) == SMK_DONE) {
      if (!loop_) {
        return false;
      }
      smk_first(video_);
    }
    repaint_ = true;
    return true;
  }
};

class MediaState : public GameState {
public:
  MediaState(GameStatePtr next)
    : nextState_(next)
  {
  }
  GameStatePtr nextState_;
};

class VideoState : public MediaState {
public:
  VideoState(const char *path, bool canClose, bool loop, GameStatePtr next)
    : MediaState(next)
    , video_(path, loop)
  {
    canClose_ = canClose;
  }

  void onActivate() override {
    get_palette(prevPalette_);
    sfx_stop();
    //effects_play_sound("Sfx\\Misc\\blank.wav");
  }
  void onDeactivate() override {
    video_.stop();
    set_palette(prevPalette_);
  }

  void onKey(const KeyEvent &e) override {
    if (e.action == KeyEvent::Press && canClose_) {
      activate(nextState_);
    }
  }

  void onMouse(const MouseEvent &e) override {
    if (e.action == MouseEvent::Press && canClose_) {
      activate(nextState_);
    }
  }

  void onRender(unsigned int time) override {
    if (!video_.render(time)) {
      activate(nextState_);
    }
  }

private:
  SmkVideo video_;
  bool canClose_;
  unsigned int firstFrame_ = 0;
  PALETTEENTRY prevPalette_[256];
};

class MusicState : public MediaState {
public:
  MusicState(int nTrack, GameStatePtr next)
    : MediaState(next)
    , nTrack_(nTrack)
  {
  }

  void onActivate() override {
    music_start(nTrack_);
    activate(nextState_);
  }

  void onKey(const KeyEvent &e) override {}

  void onMouse(const MouseEvent &e) override {}

  void onRender(unsigned int time) override {}

private:
  int nTrack_;
};

GameStatePtr get_video_state(const char *path, bool allowSkip, bool loop, GameStatePtr next) {
  return new VideoState(path, allowSkip, loop, next);
}

GameStatePtr get_music_state(int nTrack, GameStatePtr next) {
  return new MusicState(nTrack, next);
}

void queue_media_state(MediaState* state) {
  auto next = GameState::current();
  MediaState* last = nullptr;
  while (auto* video = dynamic_cast<MediaState*>(next)) {
    last = video;
    next = video->nextState_;
  }
  state->nextState_ = next;
  if (last) {
    last->nextState_ = state;
  } else {
    GameState::activate(state);
  }
}

void queue_video_state(const char *path, bool allowSkip, bool loop) {
  queue_media_state(new VideoState(path, allowSkip, loop, nullptr));
}

void queue_music_state(int nTrack) {
  queue_media_state(new MusicState(nTrack, nullptr));
}
