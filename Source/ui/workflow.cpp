#include "../diablo.h"
#include "selhero.h"
#include "../storm/storm.h"

#include "network.h"

#ifdef SPAWN
#define NO_CLASS "The Rogue and Sorcerer are only available in the full retail version of Diablo. For ordering information call (800) 953-SNOW."
#endif
#define NO_NETWORK "No network selected. Exit game and configure connection on the front page."

GameStatePtr select_name_dialog(const _uiheroinfo& info, std::function<void(const char*)>&& next) {
  return select_string_dialog(info, gbMaxPlayers > 1 ? "New Multi Player Hero" : "New Single Player Hero", "Enter Name", std::move(next));
}

GameStatePtr select_load_dialog(const _uiheroinfo& info, std::function<void(int)>&& next) {
  return select_twoopt_dialog(info, "Single Player Characters", "Save File Exists", "Load Game", "New Game", std::move(next));
}

void multiplayer_name_dialog(_uiheroinfo hero, int difficulty) {
  GameStatePtr prev = GameState::current();
  GameState::activate(select_string_dialog(hero, "Create Game", "Enter Name", [hero, difficulty, prev](const char* name) {
    if (name) {
      GameState::activate(get_network_state(hero.name, name, difficulty));
    } else {
      GameState::activate(prev);
    }
  }));
}

void multiplayer_diff_dialog(_uiheroinfo hero) {
  GameStatePtr prev = GameState::current();
  GameState::activate(select_diff_dialog(hero, "Create Game", [hero, prev](int value) {
    if (value < 0) {
      GameState::activate(prev);
    } else {
      multiplayer_name_dialog(hero, value);
    }
  }));
}

void multiplayer_dialog(_uiheroinfo hero) {
  // don't return to prev state, because it might be hero creation
  GameState::activate(select_twoopt_dialog(hero, "Multi Player Characters", "Select Action", "Create Game", "Join Game", [hero](int value) {
    if (value < 0) {
      start_game_dialog();
    } else if (value == 0) {
      multiplayer_diff_dialog(hero);
    } else {
      multiplayer_name_dialog(hero, -1);
    }
  }));
}

void name_dialog(_uiheroinfo hero) {
  GameStatePtr prev = GameState::current();
  GameState::activate(select_name_dialog(hero, [hero, prev](const char* name) mutable {
    if (name) {
      strcpy(hero.name, name);
      pfile_ui_save_create(&hero);
      if (gbMaxPlayers == 1) {
        GameState::activate(get_play_state(hero.name, SELHERO_NEW_DUNGEON));
      } else {
        multiplayer_dialog(hero);
      }
    } else {
      GameState::activate(prev);
    }
  }));
}

void create_dialog() {
  GameStatePtr prev = GameState::current();
  GameState::activate(select_class_dialog([prev](_uiheroinfo* hero) {
    if (!hero) {
      GameState::activate(prev);
    } else {
#ifdef SPAWN
      if (hero->heroclass > 0) {
        GameState::activate(get_ok_dialog(NO_CLASS, GameState::current(), false));
        return;
      }
#endif
      name_dialog(*hero);
    }
  }));
}

void newgame_dialog(_uiheroinfo hero) {
  if (hero.level >= MIN_NIGHTMARE_LEVEL) {
    GameStatePtr prev = GameState::current();
    GameState::activate(select_diff_dialog(hero, "New Game", [hero, prev](int diff) {
      if (diff < 0) {
        GameState::activate(prev);
      } else {
        gnDifficulty = diff;
        GameState::activate(get_play_state(hero.name, SELHERO_NEW_DUNGEON));
      }
    }));
  } else {
    GameState::activate(get_play_state(hero.name, SELHERO_NEW_DUNGEON));
  }
}

void hero_selected_dialog(_uiheroinfo hero) {
  if (gbMaxPlayers == 1) {
    if (hero.hassaved) {
      GameStatePtr prev = GameState::current();
      GameState::activate(select_load_dialog(hero, [hero, prev](int value) {
        if (value < 0) {
          GameState::activate(prev);
        } else if (value == 0) {
          GameState::activate(get_play_state(hero.name, SELHERO_CONTINUE));
        } else if (value == 1) {
          newgame_dialog(hero);
        }
      }));
    } else {
      newgame_dialog(hero);
    }
  } else {
    multiplayer_dialog(hero);
  }
}

void start_game_dialog() {
  auto heroes = pfile_ui_set_hero_infos();
  if (heroes.empty()) {
    create_dialog();
  } else {
    GameState::activate(select_hero_dialog(heroes, [heroes](int index) {
      if (index == -1) {
        GameState::activate(get_main_menu_dialog());
      } else if (index == -2) {
        start_game_dialog();
      } else if (index >= 0 && index < (int) heroes.size()) {
        strcpy(gszHero, heroes[index].name);
        hero_selected_dialog(heroes[index]);
      } else if (index == (int) heroes.size()) {
        create_dialog();
      }
    }));
  }
}

void start_game(bool multiplayer) {
  gnDifficulty = 0;
  if (multiplayer && !SNet_HasMultiplayer()) {
    GameState::activate(get_ok_dialog(NO_NETWORK, get_main_menu_dialog(), false));
    return;
  }
  gbMaxPlayers = (multiplayer ? MAX_PLRS : 1);
  SetRndSeed(0);
  memset(plr, 0, sizeof(plr));
  start_game_dialog();
}
