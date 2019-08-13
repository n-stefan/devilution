#include "common.h"
#include "dialog.h"

#ifdef SPAWN
#define NO_INTRO "The Diablo introduction cinematic is only available in the full retail version of Diablo. For ordering information call (800) 953-SNOW."
#endif

enum {
  MM_SINGLEPLAYER,
  MM_MULTIPLAYER,
  MM_REPLAYINTRO,
  MM_SHOWCREDITS,
  MM_EXITDIABLO,
};

int menu_music_track_id = TMUSIC_INTRO;

void mainmenu_refresh_music() {
  music_start(menu_music_track_id);
  do {
    menu_music_track_id++;
    if (menu_music_track_id == NUM_MUSIC)
      menu_music_track_id = 0;
  } while (!menu_music_track_id || menu_music_track_id == 1);
}

class MainMenuDialog : public DialogState {
public:
  MainMenuDialog() {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{64, 192, 574, 235}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_SINGLEPLAYER, "Single Player"});
    addItem({{64, 235, 574, 278}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_MULTIPLAYER, "Multi Player"});
    addItem({{64, 277, 574, 320}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_REPLAYINTRO, "Replay Intro"});
    addItem({{64, 320, 574, 363}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_SHOWCREDITS, "Show Credits"});
    addItem({{64, 363, 574, 406}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_EXITDIABLO, "Exit Diablo"});
    addItem({{17, 444, 622, 465}, ControlType::Text, ControlFlags::Small, 0, gszProductName});
    addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});
  }

  void onActivate() override {
    mainmenu_refresh_music();
#ifdef SPAWN
    LoadBackgroundArt("ui_art\\swmmenu.pcx");
#else
    LoadBackgroundArt("ui_art\\mainmenu.pcx");
#endif
  }

  void onKey(const KeyEvent& e) override {
    DialogState::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      GameState::activate(nullptr);
    }
  }

  void onInput(int index) override {
    switch (index) {
    case MM_SINGLEPLAYER:
      UiPlaySelectSound();
      start_game(false);
      break;
    case MM_MULTIPLAYER:
      UiPlaySelectSound();
      start_game(true);
      break;
    case MM_REPLAYINTRO:
      UiPlaySelectSound();
#ifndef SPAWN
      music_stop();
      GameState::activate(get_video_state("gendata\\diablo1.smk", true, false,
                                          get_main_menu_dialog()));
#else
      GameState::activate(get_ok_dialog(NO_INTRO, get_main_menu_dialog(), true));
#endif
      break;
    case MM_SHOWCREDITS:
      UiPlaySelectSound();
      GameState::activate(get_credits_dialog());
      break;
    case MM_EXITDIABLO:
      UiPlaySelectSound();
      GameState::activate(nullptr);
      break;
    }
  }
};

GameStatePtr get_main_menu_dialog() {
  return new MainMenuDialog();
}
