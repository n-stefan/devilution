#include "../diablo.h"
#include "storm.h"

#include <memory>
#include <vector>
#include <deque>

typedef std::vector<BYTE> buffer_t;
static std::deque<buffer_t> messages_;
static buffer_t lastMessage_;

BOOL  _SNetReceiveMessage(int *senderplayerid, char **data, int *databytes) {
  if (messages_.empty()) {
    SErrSetLastError(STORM_ERROR_NO_MESSAGES_WAITING);
    return FALSE;
  }

  lastMessage_ = std::move(messages_.front());
  messages_.pop_front();
  *senderplayerid = 0;
  *databytes = (int) lastMessage_.size();
  *data = (char*) lastMessage_.data();
  return TRUE;
}

BOOL  _SNetSendMessage(int playerID, void *data, unsigned int databytes) {
  if (playerID == 0 || playerID == SNPLAYER_ALL) {
    BYTE* raw = (BYTE*) data;
    buffer_t message(raw, raw + databytes);
    messages_.emplace_back(std::move(message));
  }
  return TRUE;
}

BOOL  _SNetReceiveTurns(int a1, int arraysize, char **arraydata, DWORD *arraydatabytes,
                      DWORD *arrayplayerstatus) {
  if (a1 != 0)
    return FALSE;
  if (arraysize != MAX_PLRS)
    return FALSE;
  return TRUE;
}

BOOL  _SNetSendTurn(char *data, unsigned int databytes) {
  return TRUE;
}

int  _SNetGetProviderCaps(struct _SNETCAPS *caps) {
  caps->size = 0;                  // engine writes only ?!?
  caps->flags = 0;                 // unused
  caps->maxmessagesize = 512;      // capped to 512; underflow if < 24
  caps->maxqueuesize = 0;          // unused
  caps->maxplayers = MAX_PLRS;     // capped to 4
  caps->bytessec = 1000000;        // ?
  caps->latencyms = 0;             // unused
  caps->defaultturnssec = 10;      // ?
  caps->defaultturnsintransit = 1; // maximum acceptable number
                                   // of turns in queue?
  return 1;
}

BOOL  _SNetUnregisterEventHandler(int evtype, SEVTHANDLER func) {
  return TRUE;
}

BOOL  _SNetRegisterEventHandler(int evtype, SEVTHANDLER func) {
  return TRUE;
}

BOOL  _SNetDestroy() {
  messages_.clear();
  return TRUE;
}

BOOL  _SNetDropPlayer(int playerid, DWORD flags) {
  return TRUE;
}

BOOL  _SNetGetGameInfo(int type, void *dst, unsigned int length, unsigned int *byteswritten) {
  return TRUE;
}

BOOL  _SNetLeaveGame(int type) {
  return TRUE;
}

BOOL  _SNetSendServerChatCommand(const char *command) {
  return TRUE;
}

int  _SNetInitializeProvider(unsigned long provider, struct _SNETPROGRAMDATA *client_info,
                           struct _SNETPLAYERDATA *user_info, struct _SNETUIDATA *ui_info,
                           struct _SNETVERSIONDATA *fileinfo) {
  return 0;
}

/**
 * @brief Called by engine for single, called by ui for multi
 */
BOOL  _SNetCreateGame(const char *pszGameName, const char *pszGamePassword, const char *pszGameStatString,
                      DWORD dwGameType, const char *GameTemplateData, int GameTemplateSize, int playerCount,
                      const char *creatorName, const char *a11, int *playerID) {
  *playerID = 0;
  return *playerID != -1;
}

BOOL  _SNetJoinGame(int id, char *pszGameName, char *pszGamePassword, char *playerName, char *userStats, int *playerID) {
  return FALSE;
}

/**
 * @brief Is this the mirror image of SNetGetTurnsInTransit?
 */
BOOL  _SNetGetOwnerTurnsWaiting(DWORD *turns) {
  *turns = 0;
  return TRUE;
}

BOOL  _SNetGetTurnsInTransit(int *turns) {
  *turns = 0;
  return TRUE;
}

/**
 * @brief engine calls this only once with argument 1
 */
BOOLEAN  _SNetSetBasePlayer(int) {
  return TRUE;
}

/**
 * @brief since we never signal STORM_ERROR_REQUIRES_UPGRADE the engine will not call this function
 */
BOOL  _SNetPerformUpgrade(DWORD *upgradestatus) {
  return FALSE;
}

/**
 * @brief not called from engine
 */
BOOL  _SNetSetGameMode(DWORD modeFlags, bool makePublic) {
  return FALSE;
}

void*  SMemAlloc(unsigned int amount, const char *logfilename, int logline, int defaultValue) {
  // fprintf(stderr, "%s: %d (%s:%d)\n", __FUNCTION__, amount, logfilename, logline);
  assert(amount != -1u);
  return malloc(amount);
}

BOOL  SMemFree(void *location, const char *logfilename, int logline, char defaultValue) {
  // fprintf(stderr, "%s: (%s:%d)\n", __FUNCTION__, logfilename, logline);
  assert(location);
  free(location);
  return true;
}

DWORD nLastError = 0;

DWORD  SErrGetLastError() {
  return nLastError;
}

void  SErrSetLastError(DWORD dwErrCode) {
  nLastError = dwErrCode;
}

int  SStrCopy(char *dest, const char *src, int max_length) {
  strncpy(dest, src, max_length);
  return strlen(dest);
}
