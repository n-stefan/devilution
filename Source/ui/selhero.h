#pragma once

#include "common.h"

const int MIN_NIGHTMARE_LEVEL = 20;
const int MIN_HELL_LEVEL = 30;

class SelectBaseDialog : public DialogState {
public:
  SelectBaseDialog(const char* title);

  void setStats(const _uiheroinfo* info);

  void onActivate() override;
};

GameStatePtr get_hero_dialog_int();
GameStatePtr get_newgame_dialog(const char* name);

GameStatePtr select_diff_dialog(const _uiheroinfo& info, GameStatePtr prev);
GameStatePtr select_load_dialog(const _uiheroinfo& info);
GameStatePtr select_name_dialog(GameStatePtr prev, _uiheroinfo hero);
GameStatePtr select_create_dialog(GameStatePtr prev);
GameStatePtr select_save_dialog(std::vector<_uiheroinfo>&& heroInfos);