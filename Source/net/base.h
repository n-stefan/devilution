#pragma once

#include <string>
#include <map>
#include <deque>
#include <array>
#include <memory>

#include "../diablo.h"
#include "abstract_net.h"

#include "../../Server/packet.hpp"

#define PS_CONNECTED 0x10000
#define PS_TURN_ARRIVED 0x20000
#define PS_ACTIVE 0x40000

const uint8_t PLR_BROADCAST = 0xFF;

namespace net {

class base : public abstract_net {
public:
  virtual int create(std::string name, std::string passwd) = 0;
  virtual int join(std::string name, std::string passwd) = 0;

  virtual bool SNetReceiveMessage(int* sender, char** data, int* size);
  virtual bool SNetSendMessage(int dest, void* data, unsigned int size);
  virtual bool SNetReceiveTurns(char** data, unsigned int* size,
                                DWORD* status);
  virtual bool SNetSendTurn(char* data, unsigned int size);
  virtual int SNetGetProviderCaps(struct _SNETCAPS* caps);
  virtual bool SNetRegisterEventHandler(event_type evtype,
                                        SEVTHANDLER func);
  virtual bool SNetUnregisterEventHandler(event_type evtype,
                                          SEVTHANDLER func);
  virtual bool SNetLeaveGame(int type);
  virtual bool SNetDropPlayer(int playerid, DWORD flags);
  virtual bool SNetGetOwnerTurnsWaiting(DWORD* turns);
  virtual bool SNetGetTurnsInTransit(int* turns);

  virtual void poll() {}
  virtual void send(const packet& pkt) = 0;

protected:
  std::map<event_type, SEVTHANDLER> registered_handlers;

  struct message_t {
    int sender; // change int to something else in devilution code later
    std::vector<uint8_t> payload;
    message_t()
        : sender(-1), payload({}) {}
    message_t(int s, std::vector<uint8_t> p)
        : sender(s), payload(std::move(p)) {}
  };

  message_t message_last;
  std::deque<message_t> message_queue;
  std::array<uint32_t, MAX_PLRS> turn_last = {};
  std::array<std::deque<uint32_t>, MAX_PLRS> turn_queue;
  std::array<bool, MAX_PLRS> connected_table = {};

  uint8_t plr_self = PLR_BROADCAST;
  uint32_t cookie_self = 0;

  template<class packet_type>
  void handle(const packet_type& pkt);

  void run_event_handler(_SNETEVENT& ev);

private:
  uint8_t get_owner();
  void clear_msg(uint8_t plr);
};

} // namespace net
