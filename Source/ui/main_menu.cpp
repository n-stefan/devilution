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
  }

  void renderExtra(unsigned int time) override {
    DrawArt((SCREEN_WIDTH - ArtLogos[LOGO_MED].width) / 2, 0, &ArtLogos[LOGO_MED], (time / 60) % 15);
  }

  void onInput(int index) override {
    if (index == 0) {
      pfile_create_player_description("Riv", 3);
      GameState::activate(nullptr);
    }
  }
};

GameState* get_main_menu_dialog() {
  LoadBackgroundArt("ui_art\\mainmenu.pcx");
  return new MainMenuDialog();
}
