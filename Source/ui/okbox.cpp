#include "common.h"
#include "dialog.h"

class OkDialog : public DialogState {
public:
  OkDialog(const char* text, GameStatePtr next, bool background)
    : next_(next)
    , background_(background)
  {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{140, 199, 540, 376}, ControlType::Text, ControlFlags::Medium | ControlFlags::WordWrap, 0, text});
    addItem({{230, 407, 410, 449}, ControlType::List, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, 0, "OK"});
    addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});
  }

  void onActivate() override {
    LoadBackgroundArt(background_ ? "ui_art\\swmmenu.pcx" : "ui_art\\black.pcx");
  }

  void onInput(int) override {
    UiPlaySelectSound();
    activate(next_);
  }

private:
  bool background_;
  GameStatePtr next_;
};

GameStatePtr get_ok_dialog(const char* text, GameStatePtr next, bool background) {
  return new OkDialog(text, next, background);
}
