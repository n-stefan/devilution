#include "common.h"
#include "dialog.h"
#include <string>
#include "../trace.h"

#ifdef SPAWN
#define NO_CLASS "The Rogue and Sorcerer are only available in the full retail version of Diablo. For ordering information call (800) 953-SNOW."
#endif

static std::vector<_uiheroinfo> heroInfos;

static BOOL  SelHero_GetHeroInfo(_uiheroinfo *pInfo) {
  heroInfos.push_back(*pInfo);
  return TRUE;
}

class SelectBaseDialog : public DialogState {
public:
  SelectBaseDialog(const char* title) {
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

  void setStats(const _uiheroinfo* info) {
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

  void onActivate() override {
    DialogState::onActivate();
    LoadBackgroundArt("ui_art\\selhero.pcx");
  }
};

class SelectLoadDialog : public SelectBaseDialog {
public:
  SelectLoadDialog(const _uiheroinfo& info)
    : SelectBaseDialog("Single Player Characters")
    , name_(info.name)
  {
    addItem({{264, 211, 584, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, "Save File Exists"});
    addItem({{265, 285, 585, 311}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, 0, "Load Game"});
    addItem({{265, 318, 585, 344}, ControlType::List, ControlFlags::Center | ControlFlags::Medium | ControlFlags::Gold, 1, "New Game"});
    addItem({{279, 427, 419, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -1, "OK"});
    addItem({{429, 427, 569, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -2, "Cancel"});

    setStats(&info);
  }

  void onKey(const KeyEvent& e) override {
    SelectBaseDialog::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      UiPlaySelectSound();
      activate(get_single_player_dialog());
    }
  }

  void onInput(int value) override {
    switch (value) {
    case -1:
      activate(get_play_state(name_.c_str(), selected == 0 ? SELHERO_CONTINUE : SELHERO_NEW_DUNGEON));
      break;
    case -2:
      activate(get_single_player_dialog());
      break;
    case 0:
      activate(get_play_state(name_.c_str(), SELHERO_CONTINUE));
      break;
    case 1:
      activate(get_play_state(name_.c_str(), SELHERO_NEW_DUNGEON));
      break;
    }
  }

private:
  std::string name_;
};

class SelectNameDialog : public SelectBaseDialog {
public:
  SelectNameDialog(bool multiplayer, GameStatePtr prev, _uiheroinfo hero)
    : SelectBaseDialog(multiplayer ? "New Multi Player Hero" : "New Single Player Hero")
    , prevState_(prev)
    , hero_(hero)
  {
    addItem({{264, 211, 584, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, "Enter Name"});
    name_ = addItem({{265, 317, 585, 350}, ControlType::Edit, ControlFlags::List | ControlFlags::Medium | ControlFlags::Gold, 0, ""});
    ok_ = addItem({{279, 427, 419, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Disabled, -1, "OK"});
    addItem({{429, 427, 569, 462}, ControlType::Button, ControlFlags::Center | ControlFlags::VCenter | ControlFlags::Big | ControlFlags::Gold, -2, "Cancel"});

    setStats(&hero_);
  }

  void onKey(const KeyEvent& e) override {
    SelectBaseDialog::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      UiPlaySelectSound();
      activate(prevState_);
    }
    if (e.action == KeyEvent::Press && e.key == KeyCode::RETURN && !this->items[name_].text.empty()) {
      UiPlaySelectSound();
      strcpy(hero_.name, this->items[name_].text.c_str());
      pfile_ui_save_create(&hero_);
      activate(get_play_state(hero_.name, SELHERO_NEW_DUNGEON));
    }
  }

  void onInput(int value) override {
    if (value == -2) {
      UiPlaySelectSound();
      activate(prevState_);
    } else if (value == -1) {
      if (!this->items[name_].text.empty()) {
        UiPlaySelectSound();
        strcpy(hero_.name, this->items[name_].text.c_str());
        pfile_ui_save_create(&hero_);
        activate(get_play_state(hero_.name, SELHERO_NEW_DUNGEON));
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
  bool multi_;
  int name_, ok_;
  GameStatePtr prevState_;
  _uiheroinfo hero_;
};

GameStatePtr get_select_name_dialog(bool multiplayer, GameStatePtr prev, int cls) {
  _uidefaultstats defaults;
  _uiheroinfo hero;
  pfile_ui_set_class_stats(cls, &defaults);
  hero.level = 1;
  hero.heroclass = cls;
  hero.strength = defaults.strength;
  hero.magic = defaults.magic;
  hero.dexterity = defaults.dexterity;
  hero.vitality = defaults.vitality;
  return new SelectNameDialog(multiplayer, prev, hero);
}

class SelectCreateDialog : public SelectBaseDialog {
public:
  SelectCreateDialog(bool multiplayer, GameStatePtr prev)
    : SelectBaseDialog(multiplayer ? "New Multi Player Hero" : "New Single Player Hero")
    , prevState_(prev)
    , multi_(multiplayer)
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

  void onKey(const KeyEvent& e) override {
    SelectBaseDialog::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      UiPlaySelectSound();
      activate(prevState_);
    }
  }

  void onSelect(int value) {
    if (value >= 0 && value < NUM_CLASSES) {
      UiPlaySelectSound();
#ifdef SPAWN
      if (value > 0) {
        activate(get_ok_dialog(NO_CLASS, this, false));
        return;
      }
#endif
      activate(new SelectNameDialog(multi_, this, hero_));
    }
  }

  void onInput(int value) override {
    if (value == -1) {
      onSelect(selected);
    } else if (value == -2) {
      UiPlaySelectSound();
      activate(prevState_);
    } else {
      onSelect(value);
    }
  }

private:
  bool multi_;
  GameStatePtr prevState_;
  _uiheroinfo hero_;
};

class SelectSaveDialog : public SelectBaseDialog {
public:
  SelectSaveDialog(bool multiplayer, std::vector<_uiheroinfo>&& heroInfos)
    : SelectBaseDialog(multiplayer ? "Multi Player Characters" : "Single Player Characters")
    , multi_(multiplayer)
    , heroes_(std::move(heroInfos))
  {
    addItem({{264, 211, 584, 244}, ControlType::Text, ControlFlags::Center | ControlFlags::Big, 0, "Select Hero"});
    addItem({{239, 429, 359, 464}, ControlType::Button, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, -1, "OK"});
    deleteIdx_ = addItem({{364, 429, 484, 464}, ControlType::Button, ControlFlags::Center | ControlFlags::Big | ControlFlags::Disabled, -2, "Delete"});
    addItem({{489, 429, 609, 464}, ControlType::Button, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, -3, "Cancel"});

    for (int i = 0; i < (int)heroes_.size(); ++i) {
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

  void onKey(const KeyEvent& e) override {
    SelectBaseDialog::onKey(e);
    if (e.action == KeyEvent::Press && e.key == KeyCode::ESCAPE) {
      UiPlaySelectSound();
      activate(get_main_menu_dialog());
    }
  }

  void onSelectHero(int value) {
    UiPlaySelectSound();
    if (value >= 0 && value < (int) heroes_.size()) {
      strcpy(gszHero, heroes_[value].name);
      if (heroes_[value].hassaved) {
        activate(new SelectLoadDialog(heroes_[value]));
      } else {
        activate(get_play_state(heroes_[value].name, SELHERO_NEW_DUNGEON));
      }
    } else {
      activate(new SelectCreateDialog(multi_, this));
    }
  }

  void onInput(int value) override {
    if (value == -1) {
      onSelectHero(selected);
    } else if (value == -2) {
      if (selected >= 0 && selected < (int) heroes_.size()) {
        UiPlaySelectSound();
        _uiheroinfo hero = heroes_[selected];
        activate(get_yesno_dialog(multi_ ? "Delete Multi Player Hero" : "Delete Single Player Hero",
                                  fmtstring("Are you sure you want to delete the character \"%s\"?", heroes_[selected].name).c_str(),
                                  [hero](bool value) mutable {
          if (value) {
            pfile_delete_save(&hero);
          }
          //TODO: multiplayer
          activate(get_single_player_dialog());
        }));
      }
    } else if (value == -3) {
      UiPlaySelectSound();
      activate(get_main_menu_dialog());
    } else {
      onSelectHero(value);
    }
  }

private:
  bool multi_;
  std::vector<_uiheroinfo> heroes_;
  int deleteIdx_;
};

GameStatePtr get_single_player_dialog() {
  heroInfos.clear();
  pfile_ui_set_hero_infos(SelHero_GetHeroInfo);
  if (!heroInfos.empty()) {
    return new SelectSaveDialog(false, std::move(heroInfos));
  } else {
    return new SelectCreateDialog(false, get_main_menu_dialog());
  }
}
