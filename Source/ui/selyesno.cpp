#include "common.h"
#include "dialog.h"

class YesNoDialog : public DialogState {
public:
  YesNoDialog(const char *title, const char *text, GameState* next, bool* result) {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{24, 161, 614, 196}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, title});
    addItem({{120, 236, 400, 404}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, text});
    addItem({{230, 390, 410, 425}, ControlType::List, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, 0, "Yes"});
    addItem({{230, 426, 410, 461}, ControlType::List, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, 1, "No"});
  }

  void onActivate() override {
    mainmenu_refresh_music();

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
  GameState* next
};

GameState* get_title_dialog() {
  return new TitleDialog();
}
