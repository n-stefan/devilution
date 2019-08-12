#include "dialog.h"
#include "../trace.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
EM_JS(void, api_open_keyboard, (int x0, int y0, int x1, int y1), {
  self.DApi.open_keyboard(x0, y0, x1, y1);
});
EM_JS(void, api_close_keyboard, (), {
  self.DApi.close_keyboard();
});

extern "C" {
  EMSCRIPTEN_KEEPALIVE void DApi_SyncText(int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9, int c10, int c11, int c12, int c13, int c14);
}

EMSCRIPTEN_KEEPALIVE void DApi_SyncText(int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9, int c10, int c11, int c12, int c13, int c14) {
  auto ptr = dynamic_cast<DialogState*>(GameState::current());
  char text[16] = {(char) c0, (char) c1, (char) c2, (char) c3, (char) c4, (char) c5, (char) c6,
    (char) c7, (char) c8, (char) c9, (char) c10, (char) c11, (char) c12, (char) c13, (char) c14};
  text[15] = 0;
  if (ptr) {
    ptr->syncText(text);
  }
}

#else
void api_open_keyboard(int x0, int y0, int x1, int y1) {
}
void api_close_keyboard() {
}
#endif

namespace {

void DrawSelector(const DialogState::Item& item, unsigned int time) {
  int size = FOCUS_SMALL;
  if (item.rect.bottom - item.rect.top >= 42) {
    size = FOCUS_BIG;
  } else if (item.rect.bottom - item.rect.top >= 30) {
    size = FOCUS_MED;
  }

  int frame = (time / 60) % 8;
  int y = (item.rect.top + item.rect.bottom - ArtFocus[size].height) / 2;

  DrawArt(item.rect.left, y, &ArtFocus[size], frame);
  DrawArt(item.rect.right - ArtFocus[size].width, y, &ArtFocus[size], frame);
}

int GetStrWidth(const char* text, size_t count, int size) {
  int strWidth = 0;

  for (size_t i = 0; i < count; i++) {
    BYTE chr = (BYTE) text[i];
    BYTE w = FontTables[size][chr + 2];
    if (w) {
      strWidth += w;
    } else {
      strWidth += FontTables[size][0];
    }
  }

  return strWidth;
}

int TextAlignment(int width, const char* text, size_t count, TXT_JUST align, _artFontTables size) {
  if (align != JustLeft) {
    int w = GetStrWidth(text, count, size);
    if (align == JustCentre) {
      return (width - w) / 2;
    } else if (align == JustRight) {
      return width - w;
    }
  }
  return 0;
}

std::string wordWrap( const std::string &text, _artFontTables size, int width ) {
  std::string result;
  int x = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    char chr = text[i];
    if (chr == ' ') {
      size_t next = i + 1;
      while (next < text.size() && !isspace((BYTE)text[next])) {
        ++next;
      }
      if (x + GetStrWidth(text.c_str() + i, next - i, size) >= width) {
        chr = '\n';
      }
    }
    if (chr == '\n') {
      x = 0;
    } else {
      BYTE w = FontTables[size][chr + 2];
      if (w == 0) {
        w = FontTables[size][0];
      }
      x += w;
    }
    result.push_back(chr);
  }
  return result;
}

void DrawArtStr(const DialogState::Item& item, unsigned int time) {
  _artFontTables size = AFT_SMALL;
  _artFontColors color = (item.flags & ControlFlags::Gold) ? AFC_GOLD : AFC_SILVER;
  TXT_JUST align = JustLeft;

  if (item.flags & ControlFlags::Medium) {
    size = AFT_MED;
  } else if (item.flags & ControlFlags::Big) {
    size = AFT_BIG;
  } else if (item.flags & ControlFlags::Huge) {
    size = AFT_HUGE;
  }

  if (item.flags & ControlFlags::Center) {
    align = JustCentre;
  } else if (item.flags & ControlFlags::Right) {
    align = JustRight;
  }

  std::string text = item.text;
  if (item.flags & ControlFlags::WordWrap) {
    text = wordWrap(text, size, item.rect.right - item.rect.left);
  }

  int x = item.rect.left + TextAlignment(item.rect.right - item.rect.left, text.c_str(), text.size(), align, size);

  int sx = x;
  int sy = item.rect.top;
  if (item.flags & ControlFlags::VCenter) {
    sy = (item.rect.top + item.rect.bottom - ArtFonts[size][color].height) / 2;
  }

  for (BYTE chr : text) {
    if (chr == '\n') {
      sx = x;
      sy += ArtFonts[size][color].height;
      continue;
    }
    BYTE w = FontTables[size][chr + 2];
    if (w == 0) {
      w = FontTables[size][0];
    }
    DrawArt(sx, sy, &ArtFonts[size][color], chr, w);
    sx += w;
  }

  if (item.type == ControlType::Edit && ((time / 500) % 2)) {
    DrawArt(sx, sy, &ArtFonts[size][color], (BYTE) '|');
  }
}

void DrawEditBox(const DialogState::Item& item, unsigned int time) {
  DrawSelector(item, time);
  DialogState::Item temp(item);
  temp.rect.left += 43;
  temp.rect.top += 1;
  temp.rect.right -= 43;
  DrawArtStr(temp, time);
}

}

void DialogState::onRender(unsigned int time) {
  lock_buf(1);
  for (const auto& item : items) {
    if (item.flags & ControlFlags::Hidden) {
      continue;
    }
    switch (item.type) {
    case ControlType::Edit:
      DrawEditBox(item, time);
      break;
    case ControlType::List:
      if (item.text.empty()) {
        continue;
      }
      if (item.value == selected) {
        DrawSelector(item, time);
      }
    case ControlType::Button:
    case ControlType::Text:
      DrawArtStr(item, time);
      break;
    case ControlType::Image:
      if ( item.value < 0 )
      {
        int frame = ( time / ( -item.value ) ) % item.image->frames;
        DrawArt( item.rect.left, item.rect.top, item.image, frame, item.rect.right - item.rect.left );
      }
      else
      {
        DrawArt( item.rect.left, item.rect.top, item.image, item.value, item.rect.right - item.rect.left );
      }
      break;
    }
  }
  renderExtra(time);
  if (cursor && !hide_cursor) {
    DrawArt(mouseX(), mouseY(), &ArtCursor);
  }
  UiFadeIn(time);
  unlock_buf(1);
  if (draw_lock(nullptr)) {
    draw_blit(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    draw_unlock();
    draw_flush();
  }
}

void DialogState::onActivate() {
  for (auto& item : items) {
    if (item.type == ControlType::Edit) {
      api_open_keyboard(item.rect.left, item.rect.top, item.rect.right, item.rect.bottom);
      break;
    }
  }
}

void DialogState::onDeactivate() {
  api_close_keyboard();
}

void DialogState::syncText(const char* text) {
  std::string text2;
  for (size_t i = 0; text[i]; ++i) {
    unsigned char code = (unsigned char) text[i];
    if (code >= 32 && code <= 127 && isprint(code)) {
      text2.push_back(code);
    }
  }
  for (auto& item : items) {
    if (item.type == ControlType::Edit) {
      item.text = text2;
      onInput(item.value);
      break;
    }
  }
}

void DialogState::setFocus_(int index, bool wrap) {
  int minVal = 1000, maxVal = -1000;
  for (auto& item : items) {
    if (item.type == ControlType::List && !item.text.empty()) {
      if (item.value < minVal) {
        minVal = item.value;
      }
      if (item.value > maxVal) {
        maxVal = item.value;
      }
    }
  }
  if (wrap) {
    if (index < minVal) {
      index = maxVal;
    } else if (index > maxVal) {
      index = minVal;
    }
  } else if (index < minVal) {
    index = minVal;
  } else if (index > maxVal) {
    index = maxVal;
  }
  if (index != selected) {
    selected = index;
    UiPlayMoveSound();
    onFocus(index);
  }
}

void DialogState::onMouse(const MouseEvent& event) {
  for (const auto& item : items) {
    if (item.type != ControlType::List && item.type != ControlType::Button && item.type != ControlType::Edit) {
      continue;
    }
    if (event.x >= item.rect.left && event.y >= item.rect.top && event.x < item.rect.right && event.y < item.rect.bottom) {
      if (item.type == ControlType::Edit) {
        if (event.action == MouseEvent::Press) {
          api_open_keyboard(item.rect.left, item.rect.top, item.rect.right, item.rect.bottom);
        }
      } else if (item.type == ControlType::List) {
        if (!item.text.empty()) {
          if (event.action == MouseEvent::Press) {
            if (!doubleclick || (item.value == selected && _GetTickCount() - prevClick_ < 1000)) {
              selected = item.value;
              onInput(item.value);
            } else {
              prevClick_ = _GetTickCount();
              setFocus_(item.value, false);
            }
          }
        }
      } else {
        if (event.action == MouseEvent::Press && event.button == KeyCode::LBUTTON) {
          onInput(item.value);
        }
      }
    }
  }
}

void DialogState::onKey(const KeyEvent& e) {
  if (e.action == KeyEvent::Press) {
    switch (e.key) {
    case KeyCode::UP:
      setFocus_(selected - 1, wraps);
      break;
    case KeyCode::DOWN:
      setFocus_(selected + 1, wraps);
      break;
    case KeyCode::TAB:
      if (e.modifiers & ModifierKey::SHIFT) {
        setFocus_(selected - 1, wraps);
      } else {
        setFocus_(selected + 1, wraps);
      }
      break;
    case KeyCode::PRIOR:
      setFocus_(-1000, false);
      break;
    case KeyCode::NEXT:
      setFocus_(1000, false);
      break;
    case KeyCode::RETURN:
    case KeyCode::SPACE:
      if ( selected != SELECT_NONE )
      {
        onInput( selected );
      }
      break;
    case KeyCode::BACK:
      for (auto &item : items) {
        if (item.type == ControlType::Edit) {
          if (!item.text.empty()) {
            item.text.pop_back();
            onInput(item.value);
          }
          break;
        }
      }
      break;
    case KeyCode::ESCAPE:
      onCancel();
      break;
    }
  }
}

void DialogState::onChar(char chr) {
  for (auto &item : items) {
    if (item.type == ControlType::Edit) {
      if (item.text.size() < 15) {
        item.text.push_back(chr);
        onInput(item.value);
      }
      break;
    }
  }
}
