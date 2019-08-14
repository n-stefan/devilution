#pragma once

#include "dialog.h"
#include <vector>
#include <string>

class NetworkState : public DialogState {
public:
  virtual void onClosed() {}
  virtual void onGameList(const std::vector<std::pair<std::string, uint32_t>>& games) {}
  virtual void onJoinAccept(int player, uint64_t init_info) {}
  virtual void onJoinReject(int reason) {}
};

GameStatePtr get_network_state(const char* name, const char* game, int difficulty);

void start_multiplayer();
