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

protected:
  void addItem(Item&& item) {
    items_.emplace_back(std::move(item));
    if (item.type == ControlType::List && selected < 0) {
      selected = item.value;
    }
  }
  const std::string& getText(int id) {
    return items_[id].text;
  }

  int selected = -1;
  bool wraps = true;
  bool cursor = true;

  virtual void onInput(int id) {};
  virtual void onFocus(int value) {};

  virtual void onRender(unsigned int time) override;
  virtual void onMouse(const MouseEvent& e) override;
  virtual void onKey(const KeyEvent& e) override;

  virtual void renderExtra(unsigned int time) {};

private:
  std::vector<Item> items_;
  int mouseX_ = 0, mouseY_ = 0;
  void setFocus_(int index, bool wrap);
};
