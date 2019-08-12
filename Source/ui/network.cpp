#include "network.h"
#include "../storm/storm.h"
#include "../../Server/packet.hpp"

static const char* sGetReason(RejectionReason reason) {
  switch (reason) {
  case JOIN_ALREADY_IN_GAME:
    return "Already in game";
  case JOIN_GAME_NOT_FOUND:
    return "Game not found";
  case JOIN_INCORRECT_PASSWORD:
    return "Incorrect password";
  case JOIN_VERSION_MISMATCH:
    return "Version mismatch";
  case JOIN_GAME_FULL:
    return "Game is full";
  case CREATE_GAME_EXISTS:
    return "Game already exists";
  default:
    return "Unknown error";
  }
}

class JoinGameState : public NetworkState {
public:
  JoinGameState(std::string const& name)
    : name_(name)
  {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    textLine_ = addItem({{140, 199, 540, 376}, ControlType::Text, ControlFlags::Medium, 0, "Connecting"});
    addItem({{230, 407, 410, 449}, ControlType::Button, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, 0, "Cancel"});
    addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});
  }

  void onCancel() override {
    UiPlaySelectSound();
    SNetLeaveGame(0);
    start_game(true);
  }

  void onClosed() override {
    activate(get_ok_dialog("Connection closed", []() {
      start_game(true);
    }));
  }

  void onInput(int value) {
    if (value == 0) {
      onCancel();
    }
  }

  void onJoinAccept(int player) override {
    // do stuff
    myplr = player;
    activate(get_play_state(name_.c_str(), SELHERO_CONNECT));
  }

  void onJoinReject(int reason) override {
    activate(get_ok_dialog(sGetReason((RejectionReason) reason), []() {
      start_game(true);
    }));
  }

  void onRender(unsigned int time) override {
    std::string text = "Connecting";
    for (int i = (time / 500) % 4; i > 0; --i) {
      text.push_back('.');
    }
    items[textLine_].text = text;
    NetworkState::onRender(time);
  }

private:
  std::string name_;
  int textLine_;
};

GameStatePtr get_network_state(const char* name, const char* game, int difficulty) {
  if (difficulty < 0) {
    SNet_JoinGame(game, "");
  } else {
    SNet_CreateGame(game, "", difficulty);
  }
  return new JoinGameState(name);
}
