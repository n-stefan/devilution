#include "selhero.h"
#include "../diablo.h"
#include "dialog.h"
#include <string>
#include "../trace.h"

class SelectDiffDialog : public DialogState {
public:
  SelectDiffDialog(const _uiheroinfo& info, GameStatePtr prev)
    : info_(info)
    , prevState_(prev) {
    bool hasNm = info.level > MIN_NIGHTMARE_LEVEL;
    bool hasHe = info.level > MIN_HELL_LEVEL;
    int enabled = ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold;
    int disabled = ControlFlags::Center | ControlFlags::Medium | ControlFlags::Silver;
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{24, 161, 614, 196}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, "New Game"});
    addItem({{300, 211, 595, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, "Select Difficulty"});
    addItem({{300, 281, 595, 307}, ControlType::List, enabled, DIFF_NORMAL, "Normal"});
    addItem({{300, 307, 595, 333}, hasNm ? ControlType::List : ControlType::Text, hasNm ? enabled : disabled, DIFF_NIGHTMARE, "Nightmare"});
    addItem({{300, 333, 595, 359}, hasHe ? ControlType::List : ControlType::Text, hasHe ? enabled : disabled, DIFF_HELL, "Hell"});
    addItem({{300, 427, 440, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -1, "OK"});
    addItem({{450, 427, 590, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -2, "Cancel"});
    typeIdx_ = addItem({{35, 211, 240, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, ""});
    descIdx_ = addItem({{35, 256, 240, 447}, ControlType::Text, ControlFlags::Small | ControlFlags::WordWrap, 0, ""});
    addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});
    doubleclick = true;

    onFocus(selected);
  }

  void onActivate() override {
    DialogState::onActivate();
    LoadBackgroundArt("ui_art\\selgame.pcx");
  }

  void onFocus(int value) override {
    switch (value) {
    case DIFF_NIGHTMARE:
      items[typeIdx_].text = "Nightmare";
      items[descIdx_].text = "Nightmare Difficulty\nThe denizens of the Labyrinth have been bolstered and will prove to be a greater challenge. This is recommended for experienced characters only.";
      break;
    case DIFF_HELL:
      items[typeIdx_].text = "Hell";
      items[descIdx_].text = "Hell Difficulty\nThe most powerful of the underworld's creatures lurk at the gateway into Hell. Only the most experienced characters should venture in this realm.";
      break;
    default:
      items[typeIdx_].text = "Normal";
      items[descIdx_].text = "Normal Difficulty\nThis is where a starting character should begin the quest to defeat Diablo.";
    }
  }

  void onKey(const KeyEvent& e) override {
    DialogState::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      UiPlaySelectSound();
      activate(prevState_ ? prevState_ : get_single_player_dialog_int());
    }
  }

  void onInput(int value) override {
    switch (value == -1 ? selected : value) {
    case -2:
      UiPlaySelectSound();
      activate(prevState_ ? prevState_ : get_single_player_dialog_int());
      break;
    case 0:
    case 1:
    case 2:
      UiPlaySelectSound();
      activate(get_play_state(info_.name, SELHERO_NEW_DUNGEON, value));
      break;
    }
  }

private:
  _uiheroinfo info_;
  GameStatePtr prevState_;
  int typeIdx_, descIdx_;
};

class SelectMultiDialog : public SelectBaseDialog {
public:
  SelectMultiDialog(const _uiheroinfo& info)
    : SelectBaseDialog("Multi Player Characters")
    , info_(info) {
    addItem({{264, 211, 584, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, "Choose Action"});
    addItem({{265, 285, 585, 311}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, 0, "Create Game"});
    addItem({{265, 318, 585, 344}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, 1, "Join Game"});
    addItem({{279, 427, 419, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -1, "OK"});
    addItem({{429, 427, 569, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -2, "Cancel"});

    setStats(&info);
  }

  void onKey(const KeyEvent& e) override {
    SelectBaseDialog::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      UiPlaySelectSound();
      activate(get_multi_player_dialog_int());
    }
  }

  void onInput(int value) override {
    switch (value == -1 ? selected : value) {
    case -2:
      UiPlaySelectSound();
      activate(get_single_player_dialog_int());
      break;
    case 0:
      UiPlaySelectSound();
      activate(get_play_state(info_.name, SELHERO_CONTINUE));
      break;
    case 1:
      UiPlaySelectSound();
      if (info_.level >= MIN_NIGHTMARE_LEVEL) {
        activate(select_diff_dialog(info_, this));
      } else {
        activate(get_play_state(info_.name, SELHERO_NEW_DUNGEON));
      }
      break;
    }
  }

private:
  _uiheroinfo info_;
};

GameStatePtr select_diff_dialog(const _uiheroinfo& info, GameStatePtr prev) {
  return new SelectDiffDialog(info, prev);
}
