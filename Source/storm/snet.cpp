#include <memory>

#include "storm.h"
#include "../net/abstract_net.h"

static std::shared_ptr<net::abstract_net> dvlnet_inst_single = net::abstract_net::make_net(net::provider_t::LOOPBACK, nullptr);
static std::shared_ptr<net::abstract_net> dvlnet_inst = dvlnet_inst_single;
static std::shared_ptr<net::abstract_net> dvlnet_inst_multi;

BOOL SNetReceiveMessage(int *senderplayerid, char **data, int *databytes) {
  if (!dvlnet_inst->SNetReceiveMessage(senderplayerid, data, databytes)) {
    SErrSetLastError(STORM_ERROR_NO_MESSAGES_WAITING);
    return false;
  }
  return true;
}

BOOL SNetSendMessage(int playerID, void *data, unsigned int databytes) {
  return dvlnet_inst->SNetSendMessage(playerID, data, databytes);
}

BOOL SNetReceiveTurns(int a1, int arraysize, char **arraydata, DWORD *arraydatabytes,
                      DWORD *arrayplayerstatus) {
  if (a1 != 0)
    ERROR_MSG("not implemented");
  if (arraysize != MAX_PLRS)
    ERROR_MSG("not implemented");
  if (!dvlnet_inst->SNetReceiveTurns(arraydata, arraydatabytes, arrayplayerstatus)) {
    SErrSetLastError(STORM_ERROR_NO_MESSAGES_WAITING);
    return false;
  }
  return true;
}

BOOL SNetSendTurn(char *data, unsigned int databytes) {
  return dvlnet_inst->SNetSendTurn(data, databytes);
}

int SNetGetProviderCaps(struct _SNETCAPS *caps) {
  return dvlnet_inst->SNetGetProviderCaps(caps);
}

BOOL SNetUnregisterEventHandler(int evtype, SEVTHANDLER func) {
  return dvlnet_inst->SNetUnregisterEventHandler(*(event_type *) &evtype, func);
}

BOOL SNetRegisterEventHandler(int evtype, SEVTHANDLER func) {
  return dvlnet_inst->SNetRegisterEventHandler(*(event_type *) &evtype, func);
}

BOOL SNetDestroy() {
  return true;
}

BOOL SNetDropPlayer(int playerid, DWORD flags) {
  return dvlnet_inst->SNetDropPlayer(playerid, flags);
}

BOOL SNetLeaveGame(int type) {
  return dvlnet_inst->SNetLeaveGame(type);
}

BOOL SNetSendServerChatCommand(const char *command) {
  return true;
}

BOOL SNetGetOwnerTurnsWaiting(DWORD *turns) {
  return dvlnet_inst->SNetGetOwnerTurnsWaiting(turns);
}

BOOL SNetGetTurnsInTransit(int *turns) {
  return dvlnet_inst->SNetGetTurnsInTransit(turns);
}

BOOL SNet_HasMultiplayer() {
  return dvlnet_inst_multi != nullptr;
}

void SNet_InitializeProvider(BOOL multiplayer) {
  if (multiplayer) {
    if (!dvlnet_inst_multi) {
      ERROR_MSG("multiplayer not initialized");
    } else {
      dvlnet_inst = dvlnet_inst_multi;
    }
  } else {
    dvlnet_inst = dvlnet_inst_single;
  }
}

void SNet_CreateGame(const char* name, const char* password, uint32_t difficulty) {
  dvlnet_inst->create(name, password, difficulty);
}

void SNet_JoinGame(const char* name, const char* password) {
  dvlnet_inst->join(name, password);
}

void SNet_Poll() {
  dvlnet_inst->poll();
}

#ifdef EMSCRIPTEN
#include <emscripten.h>
extern "C" {
EMSCRIPTEN_KEEPALIVE
#endif
void SNet_InitWebsocket() {
  dvlnet_inst_multi = net::abstract_net::make_net(net::provider_t::WEBSOCKET, "");
}
#ifdef EMSCRIPTEN
}
#endif
