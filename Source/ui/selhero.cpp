#include "selhero.h"
#include "../diablo.h"
#include "dialog.h"
#include <string>
#include "../trace.h"

SelectBaseDialog::SelectBaseDialog(const char* title) {
  addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
  addItem({{24, 161, 614, 196}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, title});
  addItem({{30, 211, 210, 287}, ControlType::Image, 0, UI_NUM_CLASSES, "", &ArtHero});
  addItem({{39, 323, 149, 344}, ControlType::Text, ControlFlags::Right, 0, "Level:"});
  addItem({{159, 323, 199, 344}, ControlType::Text, ControlFlags::Center, 0, ""});
  addItem({{39, 358, 149, 379}, ControlType::Text, ControlFlags::Right, 0, "Strength:"});
  addItem({{159, 358, 199, 379}, ControlType::Text, ControlFlags::Center, 0, ""});
  addItem({{39, 380, 149, 401}, ControlType::Text, ControlFlags::Right, 0, "Magic:"});
  addItem({{159, 380, 199, 401}, ControlType::Text, ControlFlags::Center, 0, ""});
  addItem({{39, 401, 149, 422}, ControlType::Text, ControlFlags::Right, 0, "Dexterity:"});
  addItem({{159, 401, 199, 422}, ControlType::Text, ControlFlags::Center, 0, ""});
  addItem({{39, 422, 149, 443}, ControlType::Text, ControlFlags::Right, 0, "Vitality:"});
  addItem({{159, 422, 199, 443}, ControlType::Text, ControlFlags::Center, 0, ""});
  addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});

  doubleclick = true;
}

void SelectBaseDialog::setStats(const _uiheroinfo* info) {
  if (info) {
    items[2].value = info->heroclass;
    items[4].text = std::to_string(info->level);
    items[6].text = std::to_string(info->strength);
    items[8].text = std::to_string(info->magic);
    items[10].text = std::to_string(info->dexterity);
    items[12].text = std::to_string(info->vitality);
  } else {
    items[2].value = UI_NUM_CLASSES;
    items[4].text = "--";
    items[6].text = "--";
    items[8].text = "--";
    items[10].text = "--";
    items[12].text = "--";
  }
}

void SelectBaseDialog::onActivate() {
  DialogState::onActivate();
  LoadBackgroundArt("ui_art\\selhero.pcx");
}

class SelectHeroDialog : public SelectBaseDialog {
public:
  SelectHeroDialog(const std::vector<_uiheroinfo>& heroInfos, std::function<void(int)>&& next)
      : SelectBaseDialog(gbMaxPlayers > 1 ? "Multi Player Characters" : "Single Player Characters")
      , heroes_(heroInfos)
      , next_(next)
  {
    addItem({{264, 211, 584, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, "Select Hero"});
    addItem({{239, 429, 359, 464}, ControlType::Button, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, -1, "OK"});
    deleteIdx_ = addItem({{364, 429, 484, 464}, ControlType::Button, ControlFlags::Center | ControlFlags::Big | ControlFlags::Disabled, -2, "Delete"});
    addItem({{489, 429, 609, 464}, ControlType::Button, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, -3, "Cancel"});

    for (int i = 0; i < (int) heroes_.size(); ++i) {
      addItem({{265, 256 + i * 26, 585, 282 + i * 26}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, i, heroes_[i].name});
    }
    if (heroes_.size() < 6) {
      addItem({{265, 256 + (int) heroes_.size() * 26, 585, 282 + (int) heroes_.size() * 26}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, (int) heroes_.size(), "New Hero"});
    }

    onFocus(selected);
  }

  void onFocus(int value) override {
    int baseFlags = ControlFlags::Center | ControlFlags::Big;
    if (value >= 0 && value < (int) heroes_.size()) {
      setStats(&heroes_[value]);
      items[deleteIdx_].flags = baseFlags | ControlFlags::Gold;
    } else if (value == (int) heroes_.size()) {
      setStats(nullptr);
      items[deleteIdx_].flags = baseFlags | ControlFlags::Disabled;
    }
  }

  void onCancel() override {
    UiPlaySelectSound();
    next_(-1);
  }

  void onInput(int value) override {
    if (value == -1) {
      UiPlaySelectSound();
      next_(selected);
    } else if (value == -2) {
      if (selected >= 0 && selected < (int) heroes_.size()) {
        UiPlaySelectSound();
        _uiheroinfo hero = heroes_[selected];
        activate(get_yesno_dialog(gbMaxPlayers > 1 ? "Delete Multi Player Hero" : "Delete Single Player Hero",
                                  fmtstring("Are you sure you want to delete the character \"%s\"?", hero.name).c_str(),
                                  [hero, next = next_](bool value) mutable {
                                    if (value) {
                                      pfile_delete_save(&hero);
                                    }
                                    next(-2);
                                  }));
      }
    } else if (value == -3) {
      UiPlaySelectSound();
      next_(-1);
    } else {
      UiPlaySelectSound();
      next_(value);
    }
  }

private:
  std::vector<_uiheroinfo> heroes_;
  std::function<void(int)> next_;
  int deleteIdx_;
};

GameStatePtr select_hero_dialog(const std::vector<_uiheroinfo>& heroInfos, std::function<void(int)>&& next) {
  return new SelectHeroDialog(heroInfos, std::move(next));
}

class SelectClassDialog : public SelectBaseDialog {
public:
  SelectClassDialog(std::function<void(_uiheroinfo*)>&& next)
      : SelectBaseDialog(gbMaxPlayers > 1 ? "New Multi Player Hero" : "New Single Player Hero")
      , next_(next)
  {
    addItem({{264, 211, 584, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, "Choose Class"});
    addItem({{264, 285, 584, 318}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, UI_WARRIOR, "Warrior"});
    addItem({{264, 318, 584, 351}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, UI_ROGUE, "Rogue"});
    addItem({{264, 352, 584, 385}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, UI_SORCERER, "Sorcerer"});
    addItem({{279, 427, 419, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -1, "OK"});
    addItem({{429, 427, 569, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -2, "Cancel"});

    onFocus(selected);
  }

  void onFocus(int value) override {
    _uidefaultstats defaults;
    pfile_ui_set_class_stats(value, &defaults);

    hero_.level = 1;
    hero_.heroclass = value;
    hero_.strength = defaults.strength;
    hero_.magic = defaults.magic;
    hero_.dexterity = defaults.dexterity;
    hero_.vitality = defaults.vitality;

    setStats(&hero_);
  }

  void onCancel() override {
    UiPlaySelectSound();
    next_(nullptr);
  }

  void onInput(int value) override {
    if (value == -1) {
      UiPlaySelectSound();
      onFocus(selected);
      next_(&hero_);
    } else if (value == -2) {
      UiPlaySelectSound();
      next_(nullptr);
    } else {
      UiPlaySelectSound();
      onFocus(value);
      next_(&hero_);
    }
  }

private:
  std::function<void(_uiheroinfo*)> next_;
  _uiheroinfo hero_;
};

GameStatePtr select_class_dialog(std::function<void(_uiheroinfo*)>&& next) {
  return new SelectClassDialog(std::move(next));
}

class SelectStringDialog : public SelectBaseDialog {
public:
  SelectStringDialog(const _uiheroinfo& hero, const char* title, const char* subtitle, std::function<void(const char*)>&& next)
      : SelectBaseDialog(title)
      , next_(next)
      , hero_(hero)
  {
    addItem({{264, 211, 584, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, subtitle});
    name_ = addItem({{265, 317, 585, 350}, ControlType::Edit, ControlFlags::List | ControlFlags::Medium | ControlFlags::Gold, 0, ""});
    ok_ = addItem({{279, 427, 419, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Disabled, -1, "OK"});
    addItem({{429, 427, 569, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -2, "Cancel"});

    setStats(&hero_);
  }

  void onKey(const KeyEvent& e) override {
    SelectBaseDialog::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      UiPlaySelectSound();
      next_(nullptr);
    }
    if (e.action == KeyEvent::Press && e.key == KeyCode::RETURN && !this->items[name_].text.empty()) {
      UiPlaySelectSound();
      next_(this->items[name_].text.c_str());
    }
  }

  void onInput(int value) override {
    if (value == -2) {
      UiPlaySelectSound();
      next_(nullptr);
    } else if (value == -1) {
      if (!this->items[name_].text.empty()) {
        UiPlaySelectSound();
        next_(this->items[name_].text.c_str());
      }
    } else if (value == 0) {
      int baseFlags = ControlFlags::Center | ControlFlags::Big;
      if (this->items[name_].text.empty()) {
        items[ok_].flags = baseFlags | ControlFlags::Disabled;
      } else {
        items[ok_].flags = baseFlags | ControlFlags::Gold;
      }
    }
  }

private:
  int name_, ok_;
  std::function<void(const char*)> next_;
  _uiheroinfo hero_;
};

GameStatePtr select_string_dialog(const _uiheroinfo& hero, const char* title, const char* subtitle, std::function<void(const char*)>&& next) {
  return new SelectStringDialog(hero, title, subtitle, std::move(next));
}

class SelectTwooptDialog : public SelectBaseDialog {
public:
  SelectTwooptDialog(const _uiheroinfo* info, const char* title, const char* subtitle, const char* opt1, const char* opt2, std::function<void(int)>&& next)
    : SelectBaseDialog(title)
    , next_(next)
  {
    addItem({{264, 211, 584, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, subtitle});
    addItem({{265, 285, 585, 311}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, 0, opt1});
    addItem({{265, 318, 585, 344}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, 1, opt2});
    addItem({{279, 427, 419, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -1, "OK"});
    addItem({{429, 427, 569, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -2, "Cancel"});

    setStats(info);
  }

  void onCancel() override {
    UiPlaySelectSound();
    next_(-1);
  }

  void onInput(int value) override {
    UiPlaySelectSound();
    next_(value == -1 ? selected : value);
  }

private:
  std::function<void(int)> next_;
};

GameStatePtr select_twoopt_dialog(const _uiheroinfo* info, const char* title, const char* subtitle, const char* opt1, const char* opt2, std::function<void(int)>&& next) {
  return new SelectTwooptDialog(info, title, subtitle, opt1, opt2, std::move(next));
}

class SelectDiffDialog : public DialogState {
public:
  SelectDiffDialog(const _uiheroinfo& info, const char* title, std::function<void(int)>&& next)
      : info_(info)
      , next_(next)
  {
    bool hasNm = info.level > MIN_NIGHTMARE_LEVEL;
    bool hasHe = info.level > MIN_HELL_LEVEL;
    int enabled = ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold;
    int disabled = ControlFlags::Center | ControlFlags::Medium | ControlFlags::Silver;
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    addItem({{24, 161, 614, 196}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, title});
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

  void onCancel() override {
    UiPlaySelectSound();
    next_(-1);
  }

  void onInput(int value) override {
    UiPlaySelectSound();
    next_(value == -1 ? selected : value);
  }

private:
  _uiheroinfo info_;
  std::function<void(int)> next_;
  int typeIdx_, descIdx_;
};

GameStatePtr select_diff_dialog(const _uiheroinfo& info, const char* title, std::function<void(int)>&& next) {
  return new SelectDiffDialog(info, title, std::move(next));
}
