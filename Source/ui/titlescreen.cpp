#include "common.h"
#include "dialog.h"

class TitleDialog : public DialogState {
public:
  TitleDialog() {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{49, 410, 599, 436}, ControlType::Text, ControlFlags::Medium | ControlFlags::Center, 0, "Copyright \xA9 1996-2001 Blizzard Entertainment"});
  }
};

GameState* get_title_dialog() {
  return new TitleDialog();
}
