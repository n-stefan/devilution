#include "common.h"
#include "dialog.h"
#include <stdio.h>

class TitleDialog : public DialogState {
public:
  TitleDialog() {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{45, 182, 595, 398}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_BIG]});
    addItem({{49, 410, 599, 436}, ControlType::Text, ControlFlags::Medium | ControlFlags::Center, 0, "Copyright \xA9 1996-2001 Blizzard Entertainment"});
    cursor = false;
  }

  void onActivate() override {
    UiFadeReset();
    LoadBackgroundArt("ui_art\\title.pcx");
  }

  void onKey(const KeyEvent& e) override {
    if (e.action == KeyEvent::Press) {
      activate(get_main_menu_dialog());
    }
  }

  void onMouse(const MouseEvent& e) override {
    if (e.action == MouseEvent::Press) {
      activate( get_main_menu_dialog() );
    }
  }

  void onRender(unsigned int time) override {
    DialogState::onRender(time);
    if (!firstFrame_) {
      firstFrame_ = time;
    }
    if (time - firstFrame_ > 7000) {
      activate( get_main_menu_dialog() );
    }
  }

private:
  unsigned int firstFrame_ = 0;
};

GameStatePtr get_title_dialog() {
  return new TitleDialog();
}
