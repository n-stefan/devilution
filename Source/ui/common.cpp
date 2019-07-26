#include "common.h"
#include "../../3rdParty/Storm/Source/storm.h"

BYTE *FontTables[4];
Art ArtFonts[4][2];
Art ArtLogos[3];
Art ArtFocus[3];
Art ArtBackground;
Art ArtCursor;
Art ArtHero;

unsigned int fadeStart = 0;

void UiFadeReset() {
  fadeStart = 0;
}
void UiFadeIn(unsigned int time) {
  if (!fadeStart) {
    fadeStart = time;
  }

  int fadeValue = int(time - fadeStart) / 60 * 16;
  if (fadeValue > 256) {
    fadeValue = 256;
  }

  SetFadeLevel(fadeValue);
}

void(__stdcall *gfnSoundFunction)(char *file);


static GameStatePtr game_state;

void GameState::activate(GameState* state) {
  if (state == game_state) {
    return;
  }
  if (game_state) {
    game_state->onDeactivate();
  }
  game_state = state;
  if (game_state) {
    game_state->onActivate();
  }
}
GameState* GameState::current() {
  return game_state;
}

void GameState::render(unsigned int time) {
  if (auto state = game_state) {
    state->retain();
    state->onRender(time);
    state->release();
  }
}
void GameState::processMouse(const MouseEvent& e) {
  if (auto state = game_state) {
    state->retain();
    state->onMouse(e);
    state->release();
  }
}

void GameState::processKey(const KeyEvent &e) {
  if (auto state = game_state) {
    state->retain();
    state->onKey(e);
    state->release();
  }
}

void GameState::processChar(char chr) {
  if (auto state = game_state) {
    state->retain();
    state->onChar(chr);
    state->release();
  }
}

void DrawArt(int screenX, int screenY, Art *art, int nFrame, int drawW) {
  BYTE *src = (BYTE *) art->data + (art->width * art->height * nFrame);
  BYTE *dst = (BYTE *) gpBuffer + (SCREEN_Y + screenY) * BUFFER_WIDTH + (SCREEN_X + screenX);
  if (!drawW) {
    drawW = art->width;
  }

  for (int i = 0; i < art->height && i + screenY < SCREEN_HEIGHT; i++, src += art->width, dst += BUFFER_WIDTH) {
    for (int j = 0; j < art->width && j + screenX < SCREEN_WIDTH; j++) {
      if (j < drawW && (!art->masked || src[j] != art->mask))
        dst[j] = src[j];
    }
  }
}

void LoadArt(char *pszFile, Art *art, int frames, PALETTEENTRY *pPalette) {
  if (art == NULL || art->data != NULL)
    return;

  if (!SBmpLoadImage(pszFile, 0, 0, 0, &art->width, &art->height, 0))
    return;

  art->data = (BYTE *) malloc(art->width * art->height);
  if (!SBmpLoadImage(pszFile, pPalette, art->data, art->width * art->height, 0, 0, 0))
    return;

  if (art->data == NULL)
    return;

  art->height /= frames;
  art->frames = frames;
}

void LoadBackgroundArt(char *pszFile) {
  PALETTEENTRY pPal[256];

  free(ArtBackground.data);
  ArtBackground.data = nullptr;

  LoadArt(pszFile, &ArtBackground, 1, pPal);
  set_palette(pPal);
}

void LoadMaskedArtFont(char *pszFile, Art *art, int frames, int mask) {
  LoadArt(pszFile, art, frames);
  art->masked = true;
  art->mask = mask;
}

void LoadArtFont(char *pszFile, int size, int color) {
  LoadMaskedArtFont(pszFile, &ArtFonts[size][color], 256, 32);
}

void UiPlayMoveSound() {
  if (gfnSoundFunction)
    gfnSoundFunction("sfx\\items\\titlemov.wav");
}

void UiPlaySelectSound() {
  if (gfnSoundFunction)
    gfnSoundFunction("sfx\\items\\titlslct.wav");
}

void GameState::retain() {
  ++counter_;
}
void GameState::release() {
  if (!--counter_) {
    delete this;
  }
}
