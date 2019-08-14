#include "base.h"

#include <algorithm>
#include <cstring>

namespace net {

void base::run_event_handler(_SNETEVENT& ev) {
  auto f = registered_handlers[static_cast<event_type>(ev.eventid)];
  if (f) {
    f(&ev);
  }
}

void base::clear_msg(uint8_t plr) {
  message_queue.erase(std::remove_if(message_queue.begin(), message_queue.end(), [&](message_t& msg) {
    return msg.sender == plr;
  }), message_queue.end());
}

template<>
void base::handle(const server_join_accept_packet& pkt) {
  if (plr_self != PLR_BROADCAST) {
    return; // already have player id
  }
  plr_self = pkt.index;
  connected_table[plr_self] = true;

  _SNETEVENT ev;
  ev.eventid = EVENT_TYPE_PLAYER_CREATE_GAME;
  ev.playerid = is_creator_ ? MAX_PLRS : plr_self;
  ev.data = const_cast<decltype(pkt.init_info)*>(&pkt.init_info);
  ev.databytes = sizeof pkt.init_info;
  run_event_handler(ev);
}

template<>
void base::handle(const server_connect_packet& pkt) {
  connected_table[pkt.id] = true;
}

template<>
void base::handle(const server_disconnect_packet& pkt) {
  if (pkt.id != plr_self) {
    if (connected_table[pkt.id]) {
      _SNETEVENT ev;
      ev.eventid = EVENT_TYPE_PLAYER_LEAVE_GAME;
      ev.playerid = pkt.id;
      ev.data = const_cast<decltype(pkt.reason)*>(&pkt.reason);
      ev.databytes = sizeof pkt.reason;
      run_event_handler(ev);
      connected_table[pkt.id] = false;
      clear_msg(pkt.id);
      turn_queue[pkt.id].clear();
    }
  } else {
    //ERROR_MSG("disconnected from the game");
  }
}

template<>
void base::handle(const server_message_packet& pkt) {
  connected_table[pkt.id] = true;
  message_queue.push_back(message_t(pkt.id, pkt.payload));
}

template<>
void base::handle(const server_turn_packet& pkt) {
  connected_table[pkt.id] = true;
  turn_queue[pkt.id].push_back(pkt.turn);
}

bool base::SNetReceiveMessage(int* sender, char** data, int* size) {
  poll();
  if (message_queue.empty())
    return false;
  message_last = message_queue.front();
  message_queue.pop_front();
  *sender = message_last.sender;
  *size = message_last.payload.size();
  *data = reinterpret_cast<char*>(message_last.payload.data());
  return true;
}

bool base::SNetSendMessage(int playerID, void* data, unsigned int size) {
  if (playerID != SNPLAYER_ALL && playerID != SNPLAYER_OTHERS && (playerID < 0 || playerID >= MAX_PLRS))
    abort();

  auto raw_message = reinterpret_cast<uint8_t*>(data);
  std::vector<uint8_t> payload(raw_message, raw_message + size);

  if (playerID == plr_self || playerID == SNPLAYER_ALL) {
    message_queue.push_back(message_t(plr_self, payload));
  }

  uint8_t dest = (playerID == SNPLAYER_ALL || playerID == SNPLAYER_OTHERS ? PLR_BROADCAST : (uint8_t) playerID);
  if (dest != plr_self) {
    send(client_message_packet(dest, payload));
  }

  return true;
}

bool base::SNetReceiveTurns(char** data, DWORD* size, DWORD* status) {
  poll();
  bool all_turns_arrived = true;
  for (auto i = 0; i < MAX_PLRS; ++i) {
    status[i] = 0;
    if (connected_table[i]) {
      status[i] |= PS_CONNECTED;
      if (turn_queue[i].empty())
        all_turns_arrived = false;
    }
  }
  if (all_turns_arrived) {
    for (auto i = 0; i < MAX_PLRS; ++i) {
      if (connected_table[i]) {
        size[i] = sizeof(uint32_t);
        status[i] |= PS_ACTIVE;
        status[i] |= PS_TURN_ARRIVED;
        turn_last[i] = turn_queue[i].front();
        turn_queue[i].pop_front();
        data[i] = reinterpret_cast<char*>(&turn_last[i]);
      }
    }
    return true;
  } else {
    for (auto i = 0; i < MAX_PLRS; ++i) {
      if (connected_table[i]) {
        if (!turn_queue[i].empty()) {
          status[i] |= PS_ACTIVE;
        }
      }
    }
    return false;
  }
}

bool base::SNetSendTurn(char* data, unsigned int size) {
  assert(size == sizeof(uint32_t));
  uint32_t turn;
  std::memcpy(&turn, data, sizeof turn);
  send(client_turn_packet(turn));
  turn_queue[plr_self].push_back(turn);
  return true;
}

int base::SNetGetProviderCaps(struct _SNETCAPS* caps) {
  caps->size = 0; // engine writes only ?!?
  caps->flags = 0; // unused
  caps->maxmessagesize = 512; // capped to 512; underflow if < 24
  caps->maxqueuesize = 0; // unused
  caps->maxplayers = MAX_PLRS; // capped to 4
  caps->bytessec = 65536; // ?
  caps->latencyms = 0; // unused
  caps->defaultturnssec = 10; // ?
  caps->defaultturnsintransit = 1; // maximum acceptable number
      // of turns in queue?
  return 1;
}

bool base::SNetUnregisterEventHandler(event_type evtype, SEVTHANDLER func) {
  registered_handlers.erase(evtype);
  return true;
}

bool base::SNetRegisterEventHandler(event_type evtype, SEVTHANDLER func) {
  /*
	  engine registers handler for:
	  EVENT_TYPE_PLAYER_LEAVE_GAME
	  EVENT_TYPE_PLAYER_CREATE_GAME (*should be ok to raise it for all players*)
	  EVENT_TYPE_PLAYER_MESSAGE (for bnet? not implemented)
	  (engine uses same function for all three)
	*/
  registered_handlers[evtype] = func;
  return true;
}

bool base::SNetLeaveGame(int type) {
  send(client_leave_game_packet());
  return true;
}

bool base::SNetDropPlayer(int playerid, DWORD flags) {
  send(client_drop_player_packet((uint8_t) playerid, (LeaveReason) flags));
  return true;
}

uint8_t base::get_owner() {
  for (auto i = 0; i < MAX_PLRS; ++i) {
    if (connected_table[i]) {
      return i;
    }
  }
  return PLR_BROADCAST; // should be unreachable
}

bool base::SNetGetOwnerTurnsWaiting(DWORD* turns) {
  *turns = turn_queue[get_owner()].size();
  return true;
}

bool base::SNetGetTurnsInTransit(int* turns) {
  *turns = turn_queue[plr_self].size();
  return true;
}

} // namespace net
