#include "common.h"
#include "dialog.h"

#ifndef SPAWN
enum {
  MM_SINGLEPLAYER,
  MM_REPLAYINTRO,
  MM_SHOWCREDITS,
  MM_EXITDIABLO,
};
#else
enum {
  MM_SINGLEPLAYER,
  MM_SHOWCREDITS,
  MM_EXITDIABLO,
};
#endif

class MainMenuDialog : public DialogState {
public:
  MainMenuDialog() {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
#ifndef SPAWN
    addItem({{64, 192, 574, 235}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_SINGLEPLAYER, "Single Player"});
    addItem({{64, 235, 574, 278}, ControlType::Text, ControlFlags::Huge | ControlFlags::Silver | ControlFlags::Center, 0, "Multi Player"});
    addItem({{64, 277, 574, 320}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_REPLAYINTRO, "Replay Intro"});
    addItem({{64, 320, 574, 363}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_SHOWCREDITS, "Show Credits"});
    addItem({{64, 363, 574, 406}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_EXITDIABLO, "Exit Diablo"});
    addItem({{17, 444, 622, 465}, ControlType::Text, ControlFlags::Small, 0, "Diablo v1.09"});
#else
    addItem({{64, 235, 574, 278}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_SINGLEPLAYER, "Single Player"});
    addItem({{64, 277, 574, 320}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_SHOWCREDITS, "Show Credits"});
    addItem({{64, 320, 574, 363}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, MM_EXITDIABLO, "Exit Diablo"});
    addItem({{17, 444, 622, 465}, ControlType::Text, ControlFlags::Small, 0, "Diablo Shareware"});
#endif
    addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});
  }

  void onActivate() override {
    mainmenu_refresh_music();
    LoadBackgroundArt( "ui_art\\mainmenu.pcx" );
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
      gbMaxPlayers = 1;
      GameState::activate(get_single_player_dialog());
      break;
#ifndef SPAWN
    case MM_REPLAYINTRO:
      UiPlaySelectSound();
      GameState::activate(get_video_state("gendata\\diablo1.smk", true, false,
                                          get_main_menu_dialog()));
      break;
#endif
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
