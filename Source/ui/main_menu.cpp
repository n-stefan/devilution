#include "common.h"
#include "dialog.h"

class MainMenuDialog : public DialogState {
public:
  MainMenuDialog() {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{64, 192, 574, 235}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, 0, "Single Player"});
    addItem({{64, 235, 574, 278}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, 1, "Multi Player"});
    addItem({{64, 277, 574, 320}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, 2, "Replay Intro"});
    addItem({{64, 320, 574, 363}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, 3, "Show Credits"});
    addItem({{64, 363, 574, 406}, ControlType::List, ControlFlags::Huge | ControlFlags::Gold | ControlFlags::Center, 4, "Exit Diablo"});
    addItem({{17, 444, 622, 465}, ControlType::Text, ControlFlags::Small, 0, "Diablo v1.09"});
    addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});
  }

  void onActivate() override {
    LoadBackgroundArt( "ui_art\\mainmenu.pcx" );
  }

  void onKey(const KeyEvent& e) {
    DialogState::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      GameState::activate(nullptr);
    }
  }

  void onInput(int index) override {
    switch (index) {
    case 2:
      GameState::activate(get_video_state("gendata\\diablo1.smk", true, false,
                                          get_main_menu_dialog()));
      break;
    case 3:
      GameState::activate(get_credits_dialog());
      break;
    case 4:
      GameState::activate(nullptr);
      break;
    }
  }
};

GameState* get_main_menu_dialog() {
  return new MainMenuDialog();
}
