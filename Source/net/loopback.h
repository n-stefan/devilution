#pragma once

#include <string>
#include <deque>

#include "../diablo.h"
#include "abstract_net.h"

namespace net {

class loopback : public abstract_net {
private:
  std::deque<std::vector<uint8_t>> message_queue;
  std::vector<uint8_t> message_last;
  const int plr_single = 0;

public:
  void create(std::string addrstr, std::string passwd, uint32_t difficulty) override;
  void join(std::string addrstr, std::string passwd) override;
  bool SNetReceiveMessage(int* sender, char** data, int* size) override;
  bool SNetSendMessage(int dest, void* data, unsigned int size) override;
  bool SNetReceiveTurns(char** data, DWORD* size, DWORD* status) override;
  bool SNetSendTurn(char* data, unsigned int size) override;
  int SNetGetProviderCaps(struct _SNETCAPS* caps) override;
  bool SNetRegisterEventHandler(event_type evtype, SEVTHANDLER func) override;
  bool SNetUnregisterEventHandler(event_type evtype, SEVTHANDLER func) override;
  bool SNetLeaveGame(int type) override;
  bool SNetDropPlayer(int playerid, DWORD flags) override;
  bool SNetGetOwnerTurnsWaiting(DWORD* turns) override;
  bool SNetGetTurnsInTransit(int* turns) override;
};

} // namespace net
