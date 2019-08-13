#include "common.h"
#include <vector>
#include "../storm/storm.h"

#include "../trace.h"

// This is an awful hack, I promise I'll make a proper state queue system
class EndGameState : public GameState {
public:
  EndGameState(bool exit)
    : exit_(exit)
  {
  }

  void onKey(const KeyEvent &e) override {}

  void onMouse(const MouseEvent &e) override {}

  void onRender(unsigned int time) override {
    if (exit_) {
      activate(nullptr);
    } else {
      start_game(gbMaxPlayers > 1);
      current()->render(time);
    }
  }

private:
  bool exit_;
};


class MainState : public GameState {
public:
  void onActivate() override {
    if (started_) {
      drawpanflag = 255;
      return;
    }
    InitLevels();
    InitQuests();
    InitPortals();
    InitDungMsgs(myplr);
    unsigned int uMsg;
    if (!gbValidSaveFile || !gbLoadGame) {
      uMsg = WM_DIABNEWGAME;
    } else {
      uMsg = WM_DIABLOADGAME;
    }

    //nthread_ignore_mutex(TRUE);
    start_game(uMsg);
    /// ASSERT: assert(ghMainWnd);
    control_update_life_mana();
    msg_process_net_packets();
    gbRunGame = TRUE;
    gbProcessPlayers = TRUE;
    gbRunGameResult = TRUE;
    drawpanflag = 255;
    DrawAndBlit();
    PaletteFadeIn(8);
    drawpanflag = 255;
    gbGameLoopStartup = TRUE;
    //nthread_ignore_mutex(FALSE);
    started_ = true;
  }

  void stop() {
    if (started_) {
      started_ = false;
      if (gbMaxPlayers > 1) {
        pfile_write_hero();
      }

      pfile_flush_W();
      PaletteFadeOut(8);
      SetCursor_(0);
      ClearScreenBuffer();
      drawpanflag = 255;
      scrollrt_draw_game_screen(TRUE);
      free_game();

      if (cineflag) {
        cineflag = FALSE;
        DoEnding();
      }

      NetClose();
      //pfile_create_player_description(0, 0);
      api_current_save_id(-1);
    }
    SNetDestroy();
  }

  void onRender(unsigned int time) override {
    for (int msg : messages) {
      if (gbMaxPlayers > 1) {
        pfile_write_hero();
      }
      //nthread_ignore_mutex(TRUE);
      PaletteFadeOut(8);
      FreeMonsterSnd();
      music_stop();
      track_repeat_walk(FALSE);
      sgbMouseDown = 0;
      //ReleaseCapture();
      ShowProgress(msg);
      drawpanflag = 255;
      DrawAndBlit();
      if (gbRunGame) {
        PaletteFadeIn(8);
      }
      //nthread_ignore_mutex(FALSE);
      gbGameLoopStartup = TRUE;
      drawpanflag = 255;
    }
    messages.clear();

    if (gbRunGame) {
      diablo_color_cyc_logic();
      multi_process_network_packets();
      game_loop(gbGameLoopStartup);
      msgcmd_send_chat();
      gbGameLoopStartup = FALSE;
      DrawAndBlit();
    }

    if (!gbRunGame) {
      activate(new EndGameState(!gbRunGameResult));
      stop();
    }
  }

  void onMouse(const MouseEvent &e) override {
    MouseX = e.x;
    MouseY = e.y;
    if ((e.modifiers & ModifierKey::TOUCH) && pcurs >= CURSOR_FIRSTITEM){
      MouseX -= cursW / 2;
      MouseY -= cursH / 2;
    }
    if (e.action == MouseEvent::Press) {
      if (e.button == KeyCode::LBUTTON) {
        if (sgbMouseDown == 0) {
          sgbMouseDown = 1;
          track_repeat_walk(LeftMouseDown(e.modifiers & ModifierKey::SHIFT));
        }
      } else if (e.button == KeyCode::RBUTTON) {
        if (sgbMouseDown == 0) {
          sgbMouseDown = 2;
          RightMouseDown();
        }
      }
    } else if (e.action == MouseEvent::Release) {
      if (e.button == KeyCode::LBUTTON) {
        if (sgbMouseDown == 1) {
          sgbMouseDown = 0;
          LeftMouseUp();
          track_repeat_walk(FALSE);
        }
      } else if (e.button == KeyCode::RBUTTON) {
        if (sgbMouseDown == 2) {
          sgbMouseDown = 0;
        }
      }
    } else {
      gmenu_on_mouse_move();
      CheckCursMove();
    }
  }

  void onKey(const KeyEvent &e) override {
    if (e.action == KeyEvent::Press) {
      PressKey(e.key);
    } else {
      ReleaseKey(e.key);
    }
  }

  void onChar(char chr) override {
    PressChar(chr);
  }

  std::vector<int> messages;

private:
  bool started_ = false;
};

void post_event(int code) {
  if (auto ptr = dynamic_cast<MainState*>(GameState::current())) {
    ptr->messages.push_back(code);
  }
}

GameStatePtr get_netplay_state(int mode) {
  return new MainState;
}

GameStatePtr get_play_state(const char* name, int mode) {
  strcpy(gszHero, name);
  pfile_create_player_description(NULL, 0);
  gbLoadGame = (mode == SELHERO_CONTINUE);
  NetInit_Mid();
  NetInit_Finish();
  return new MainState;
}
