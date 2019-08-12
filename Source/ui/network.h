#pragma once

#include "common.h"
#include <vector>
#include <string>

class NetworkState : public GameState {
public:
  virtual void onClosed() {}
  virtual void onGameList(const std::vector<std::pair<std::string, uint32_t>>& games) {}
  virtual void onJoinAccept(int player) {}
  virtual void onJoinReject(int reason) {}
};
