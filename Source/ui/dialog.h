#pragma once

#include "common.h"
#include <vector>

enum class ControlType {
  Text,
  Image,
  Button,
  List,
  Edit,
};

namespace ControlFlags {
enum {
  Small       = 0x0001,
  Medium      = 0x0002,
  Big         = 0x0004,
  Huge        = 0x0008,
  Center      = 0x0010,
  Right       = 0x0020,
  VCenter     = 0x0040,
  Silver      = 0x0080,
  Gold        = 0x0100,
  Small1      = 0x0200,
  Small2      = 0x0400,
  List        = 0x0800,
  Disabled    = 0x1000,
  Hidden      = 0x2000,
  WordWrap    = 0x4000,
};
}

class DialogState : public GameState {
public:
  struct Item {
    RECT rect;
    ControlType type;
    int flags;
    int value;
    std::string text;
    Art* image;
  };

  void syncText(const char* text);

protected:
  int addItem(Item&& item) {
    items.emplace_back(std::move(item));
    if (item.type == ControlType::List && selected < 0) {
      selected = item.value;
    }
    return (int)items.size() - 1;
  }

  std::vector<Item> items;
  int selected = -1;
  bool wraps = true;
  bool cursor = true;
  bool doubleclick = false;

  virtual void onInput(int id) {};
  virtual void onFocus(int value) {};

  virtual void onActivate() override;
  virtual void onDeactivate() override;

  virtual void onRender(unsigned int time) override;
  virtual void onMouse(const MouseEvent& e) override;
  virtual void onKey( const KeyEvent& e ) override;
  virtual void onChar( char chr ) override;

  virtual void renderExtra(unsigned int time) {};

private:
  DWORD prevClick_ = 0;
  void setFocus_(int index, bool wrap);
};
