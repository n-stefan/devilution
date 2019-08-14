#include "network.h"
#include "../storm/storm.h"
#include "../../Server/packet.hpp"
#include "selhero.h"

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

#define LVL_NIGHTMARE "Your character must reach level 20 before you can enter a multiplayer game of Nightmare difficulty."
#define LVL_HELL "Your character must reach level 30 before you can enter a multiplayer game of Hell Difficulty."

Art ArtPopupSm;
Art ArtProgBG;
Art ProgFil;
Art ButImage;

class JoinGameState : public NetworkState {
public:
  JoinGameState(const char* game, int difficulty) :
    game_(game),
    difficulty_(difficulty)
  {
    addItem({{0, 0, 640, 480}, ControlType::Image, 0, 0, "", &ArtBackground});
    textLine_ = addItem({{140, 199, 540, 376}, ControlType::Text, ControlFlags::Medium, 0, "Connecting"});
    addItem({{230, 407, 410, 449}, ControlType::Button, ControlFlags::Center | ControlFlags::Big | ControlFlags::Gold, 0, "Cancel"});
    addItem({{125, 0, 515, 154}, ControlType::Image, 0, -60, "", &ArtLogos[LOGO_MED]});
  }

  void onActivate() override {
    LoadBackgroundArt("ui_art\\black.pcx");
    //LoadArt("ui_art\\spopup.pcx", &ArtPopupSm);
    //LoadArt("ui_art\\prog_bg.pcx", &ArtProgBG);
    //LoadArt("ui_art\\prog_fil.pcx", &ProgFil);
    //LoadArt("ui_art\\but_sml.pcx", &ButImage, 15);

    if (difficulty_ < 0) {
      SNet_JoinGame(game_.c_str(), "");
    } else {
      SNet_CreateGame(game_.c_str(), "", difficulty_);
    }
  }

  void onCancel() override {
    UiPlaySelectSound();
    SNetLeaveGame(0);
    start_game(true);
  }
  void onDeactivate() override {
    if (!done_) {
      NetClose();
    }
  }

  void onClosed() override {
    activate(get_ok_dialog("Connection closed", []() {
      start_game(true);
    }));
  }

  void onInput(int value) override {
    if (value == 0) {
      onCancel();
    }
  }

  void onJoinAccept(int player, uint64_t init_info) override {
    myplr = player;
    pfile_read_player_from_save();
    int difficulty = int(init_info >> 32);
    if (difficulty == DIFF_NIGHTMARE && plr[myplr]._pLevel < MIN_NIGHTMARE_LEVEL) {
      activate(get_ok_dialog(LVL_NIGHTMARE, []() {
        start_game(true);
      }));
      return;
    }
    if (difficulty == DIFF_HELL && plr[myplr]._pLevel < MIN_HELL_LEVEL) {
      activate(get_ok_dialog(LVL_HELL, []() {
        start_game(true);
      }));
      return;
    }
    gbLoadGame = FALSE;
    NetInit_Mid();
    postInit_ = true;
    if (NetInit_NeedSync()) {
      msg_wait_resync();
      postInit_ = true;
    } else {
      done();
    }
  }

  void done() {
    done_ = true;
    NetInit_Finish();
    activate(get_netplay_state(SELHERO_CONNECT));
  }

  void onJoinReject(int reason) override {
    activate(get_ok_dialog(sGetReason((RejectionReason) reason), []() {
      start_game(true);
    }));
  }

  void onRender(unsigned int time) override {
    std::string text = "Connecting";
    if (postInit_) {
      progress_ = msg_wait_for_turns();
      text += fmtstring(" (%d%%)", progress_);
    } else {
      for (int i = (time / 500) % 4; i > 0; --i) {
        text.push_back('.');
      }
    }
    items[textLine_].text = text;
    NetworkState::onRender(time);
    if (postInit_ && progress_ >= 100 && 0) {
      done();
    }
  }

private:
  std::string game_;
  int difficulty_;
  int textLine_;
  bool postInit_ = false;
  int progress_ = -1;
  bool done_ = false;
};

GameStatePtr get_network_state(const char* name, const char* game, int difficulty) {
  strcpy(gszHero, name);
  pfile_create_player_description(NULL, 0);
  multi_event_handler(TRUE);
  return new JoinGameState(game, difficulty);
}
