#include "common.h"
#include "dialog.h"

class YesNoDialog : public DialogState {
public:
  YesNoDialog(const char *title, const char *text, std::function<void(bool)>&& next) :
    next_(std::move(next))
  {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{24, 161, 614, 196}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, title});
    addItem({{120, 236, 520, 404}, ControlType::Text, ControlFlags::Medium | ControlFlags::WordWrap, 0, text});
    addItem({{230, 390, 410, 425}, ControlType::List, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, 0, "Yes"});
    addItem({{230, 426, 410, 461}, ControlType::List, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, 1, "No"});
    addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});
  }

  void onActivate() override {
    LoadBackgroundArt("ui_art\\black.pcx");
  }

  void onCancel() override {
    next_(false);
  }

  void onInput(int value) override {
    UiPlaySelectSound();
    next_(value == 0);
  }

private:
  std::function<void(bool)> next_;
};

GameStatePtr get_yesno_dialog(const char* title, const char* text, std::function<void(bool)>&& next) {
  return new YesNoDialog(title, text, std::move(next));
}
