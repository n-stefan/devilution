#pragma once

#include "common.h"
#include "dialog.h"

const int MIN_NIGHTMARE_LEVEL = 20;
const int MIN_HELL_LEVEL = 30;

class SelectBaseDialog : public DialogState {
public:
  SelectBaseDialog(const char* title);

  void setStats(const _uiheroinfo* info);

  void onActivate() override;
};

GameStatePtr select_hero_dialog(const std::vector<_uiheroinfo>& heroInfos, std::function<void(int)>&& next);
GameStatePtr select_class_dialog(std::function<void(_uiheroinfo*)>&& next);
GameStatePtr select_diff_dialog(const _uiheroinfo& info, const char* title, std::function<void(int)>&& next);
GameStatePtr select_string_dialog(const _uiheroinfo& info, const char* title, const char* subtitle, std::function<void(const char*)>&& next);
GameStatePtr select_twoopt_dialog(const _uiheroinfo* info, const char* title, const char* subtitle, const char* opt1, const char* opt2, std::function<void(int)>&& next);
