#pragma once

#include "../../types.h"
#include "../diablo.h"
#include "event.h"

typedef enum _artFocus {
  FOCUS_SMALL,
  FOCUS_MED,
  FOCUS_BIG,
} _artFocus;

typedef enum _artLogo {
  LOGO_SMALL,
  LOGO_MED,
  LOGO_BIG,
} _artLogo;

typedef enum _artFontTables {
  AFT_SMALL,
  AFT_MED,
  AFT_BIG,
  AFT_HUGE,
} _artFontTables;

typedef enum _artFontColors {
  AFC_SILVER,
  AFC_GOLD,
} _artFontColors;

typedef struct Art {
  BYTE *data;
  DWORD width;
  DWORD height;
  DWORD frames;
  bool masked = false;
  BYTE mask;
} Art;

extern BYTE *FontTables[4];
extern Art ArtFonts[4][2];
extern Art ArtLogos[3];
extern Art ArtFocus[3];
extern Art ArtBackground;
extern Art ArtCursor;
extern Art ArtHero;

void LoadArt(char *pszFile, Art *art, int frames,
             PALETTEENTRY *pPalette = nullptr);
void DrawArt(int screenX, int screenY, Art *art, int nFrame = 0, int drawW = 0);
void LoadBackgroundArt(char *pszFile);
void LoadMaskedArtFont(char *pszFile, Art *art, int frames, int mask = 250);
void LoadArtFont(char *pszFile, int size, int color);
void UiFadeReset();
void UiFadeIn(unsigned int time);

void UiPlayMoveSound();
void UiPlaySelectSound();

typedef enum TXT_JUST {
  JustLeft = 0,
  JustCentre = 1,
  JustRight = 2,
} TXT_JUST;

template <class T, size_t N> constexpr size_t size(T (&)[N]) { return N; }

extern void(__stdcall *gfnSoundFunction)(char *file);

class GameState {
public:
  virtual ~GameState(){};

  static void activate(GameState* state);
  static GameState *current();

  static void render(unsigned int time);
  static void processMouse(const MouseEvent &e);
  static void processKey(const KeyEvent &e);
  static void processChar(char chr);

  void retain();
  void release();

protected:
  virtual void onActivate(){};
  virtual void onDeactivate(){};

  virtual void onRender(unsigned int time) = 0;
  virtual void onMouse(const MouseEvent &e) = 0;
  virtual void onKey(const KeyEvent &e) = 0;
  virtual void onChar(char chr){};

private:
  int counter_ = 0;
};

class GameStatePtr {
public:
  GameStatePtr() : ptr_(nullptr) {}
  GameStatePtr(const GameStatePtr &ptr) : ptr_(ptr.ptr_) {
    if (ptr_) {
      ptr_->retain();
    }
  }
  GameStatePtr(GameStatePtr&& ptr) : ptr_(ptr.ptr_) {
    ptr.ptr_ = nullptr;
  }
  GameStatePtr(GameState *ptr) : ptr_(ptr) {
    if (ptr_) {
      ptr_->retain();
    }
  }
  ~GameStatePtr() {
    if (ptr_) {
      ptr_->release();
    }
  }

  GameStatePtr &operator=(const GameStatePtr &ptr) {
    if (ptr.ptr_) {
      ptr.ptr_->retain();
    }
    if (ptr_) {
      ptr_->release();
    }
    ptr_ = ptr.ptr_;
    return *this;
  }
  GameStatePtr &operator=(GameStatePtr &&ptr) {
    ptr_ = ptr.ptr_;
    ptr.ptr_ = nullptr;
    return *this;
  }

  operator bool() {
    return ptr_ != nullptr;
  }

  operator GameState *( ) {
    return ptr_;
  }

  GameState* operator->() {
    return ptr_;
  }

  GameState *get() {
    return ptr_;
  }

private:
  GameState *ptr_;
};

GameStatePtr get_title_dialog();
GameStatePtr get_main_menu_dialog();
GameStatePtr get_credits_dialog();
GameStatePtr get_video_state(const char *path, bool allowSkip, bool loop, GameStatePtr next);
