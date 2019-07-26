#include "common.h"
#include "dialog.h"

class TitleDialog : public DialogState {
public:
  TitleDialog() {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{49, 410, 599, 436}, ControlType::Text, ControlFlags::Medium | ControlFlags::Center, 0, "Copyright \xA9 1996-2001 Blizzard Entertainment"});
    cursor = false;
  }
  void renderExtra(unsigned int time) override {
    DrawArt((SCREEN_WIDTH - ArtLogos[LOGO_BIG].width) / 2, 182, &ArtLogos[LOGO_BIG], (time / 60) % 15);
  }

  void onKey(const KeyEvent& e) override {
    if (e.action == KeyEvent::Press) {
      activate(get_main_menu_dialog());
    }
  }
};

GameState* get_title_dialog() {
  LoadBackgroundArt("ui_art\\title.pcx");
  return new TitleDialog();
}
