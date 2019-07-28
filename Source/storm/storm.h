#pragma once

#include "../diablo.h"

typedef void ( *SEVTHANDLER)(struct _SNETEVENT *);

#ifndef __STORM_SMAX
#define __STORM_SMAX(x,y) (x < y ? y : x)
#endif

#ifndef __STORM_SSIZEMAX
#define __STORM_SSIZEMAX(x,y) (__STORM_SMAX(sizeof(x),sizeof(y)))
#endif

#ifndef __STORM_SMIN
#define __STORM_SMIN(x,y) (x < y ? x : y)
#endif

#ifndef __STORM_SSIZEMIN
#define __STORM_SSIZEMIN(x,y) (__STORM_SMIN(sizeof(x),sizeof(y)))
#endif

#define GAMESTATE_PRIVATE 0x01
#define GAMESTATE_FULL    0x02
#define GAMESTATE_ACTIVE  0x04
#define GAMESTATE_STARTED 0x08
#define GAMESTATE_REPLAY  0x80

typedef struct _WRECT {
  WORD  left;
  WORD  top;
  WORD  right;
  WORD  bottom;
} WRECT, *PWRECT;

typedef struct _WPOINT {
  WORD  x;
  WORD  y;
} WPOINT, *PWPOINT;

typedef struct _WSIZE {
  WORD  cx;
  WORD  cy;
} WSIZE, *PWSIZE;

#define SNGetGameInfo(typ,dst) SNetGetGameInfo(typ, &dst, sizeof(dst))

// Game info fields
#define GAMEINFO_NAME           1
#define GAMEINFO_PASSWORD       2
#define GAMEINFO_STATS          3
#define GAMEINFO_MODEFLAG       4
#define GAMEINFO_GAMETEMPLATE   5
#define GAMEINFO_PLAYERS        6

// Traffic flags
#define STRAFFIC_NORMAL 0
#define STRAFFIC_VERIFY 1
#define STRAFFIC_RESEND 2
#define STRAFFIC_REPLY  4

// Values for arrayplayerstatus
#define SNET_PS_OK             0
#define SNET_PS_WAITING        2
#define SNET_PS_NOTRESPONDING  3
#define SNET_PS_UNKNOWN        default


// Event structure
typedef struct _s_evt {
  DWORD dwFlags;
  int   dwPlayerId;
  void  *pData;
  DWORD dwSize;
} S_EVT, *PS_EVT;

// Macro values to target specific players
#define SNPLAYER_ALL    -1
#define SNPLAYER_OTHERS -2

#define MPQ_FLAG_READ_ONLY 1

#define SNMakeGamePublic() SNetSetGameMode( (DWORD mode, SNetGetGameInfo(GAMEINFO_MODEFLAGS, &mode, 4), mode), true)

#define SNET_GAME_RESULT_WIN        1
#define SNET_GAME_RESULT_LOSS       2
#define SNET_GAME_RESULT_DRAW       3
#define SNET_GAME_RESULT_DISCONNECT 4

// values for dwFlags
enum MPQFlags {
  MPQ_NO_LISTFILE = 0x0010,
  MPQ_NO_ATTRIBUTES = 0x0020,
  MPQ_FORCE_V1 = 0x0040,
  MPQ_CHECK_SECTOR_CRC = 0x0080
};

// values for dwSearchScope
enum SFileFlags {
  SFILE_FROM_MPQ = 0x00000000,
  SFILE_FROM_ABSOLUTE = 0x00000001,
  SFILE_FROM_RELATIVE = 0x00000002,
  SFILE_FROM_DISK = 0x00000004
};

#define SBMP_DEFAULT  0
#define SBMP_BMP      1
#define SBMP_PCX      2
#define SBMP_TGA      3

#define SMAlloc(amount) SMemAlloc((amount), __FILE__, __LINE__)
#define SMFree(loc) SMemFree((loc), __FILE__, __LINE__)

#define SMReAlloc(loc,s) SMemReAlloc((loc),(s), __FILE__, __LINE__)

// Can be provided instead of logline/__LINE__ parameter to indicate different errors.
#define SLOG_EXPRESSION    0
#define SLOG_FUNCTION     -1
#define SLOG_OBJECT       -2
#define SLOG_HANDLE       -3
#define SLOG_FILE         -4
#define SLOG_EXCEPTION    -5

#define SREG_NONE                   0x00000000
#define SREG_EXCLUDE_LOCAL_MACHINE  0x00000001  // excludes checking the HKEY_LOCAL_MACHINE hive
#define SREG_BATTLE_NET             0x00000002  // sets the relative key to "Software\\Battle.net\\" instead
#define SREG_EXCLUDE_CURRENT_USER   0x00000004  // excludes checking the HKEY_CURRENT_USER hive
#define SREG_ABSOLUTE               0x00000010  // specifies that the key is not a relative key

#define SAssert(x) { if ( !(x) ) SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, #x) }

#define SEDisplayError(err) SErrDisplayError(e, __FILE__, __LINE__)

#define SEGetErrorStr(e,b) SErrGetErrorStr(e,b,sizeof(b))

// Values for dwErrCode
#define STORM_ERROR_ASSERTION                    0x85100000
#define STORM_ERROR_BAD_ARGUMENT                 0x85100065
#define STORM_ERROR_GAME_ALREADY_STARTED         0x85100066
#define STORM_ERROR_GAME_FULL                    0x85100067
#define STORM_ERROR_GAME_NOT_FOUND               0x85100068
#define STORM_ERROR_GAME_TERMINATED              0x85100069
#define STORM_ERROR_INVALID_PLAYER               0x8510006a
#define STORM_ERROR_NO_MESSAGES_WAITING          0x8510006b
#define STORM_ERROR_NOT_ARCHIVE                  0x8510006c
#define STORM_ERROR_NOT_ENOUGH_ARGUMENTS         0x8510006d
#define STORM_ERROR_NOT_IMPLEMENTED              0x8510006e
#define STORM_ERROR_NOT_IN_ARCHIVE               0x8510006f
#define STORM_ERROR_NOT_IN_GAME                  0x85100070
#define STORM_ERROR_NOT_INITIALIZED              0x85100071
#define STORM_ERROR_NOT_PLAYING                  0x85100072
#define STORM_ERROR_NOT_REGISTERED               0x85100073
#define STORM_ERROR_REQUIRES_CODEC1              0x85100074
#define STORM_ERROR_REQUIRES_CODEC2              0x85100075
#define STORM_ERROR_REQUIRES_CODEC3              0x85100076
#define STORM_ERROR_REQUIRES_UPGRADE             0x85100077
#define STORM_ERROR_STILL_ACTIVE                 0x85100078
#define STORM_ERROR_VERSION_MISMATCH             0x85100079
#define STORM_ERROR_MEM_NOT_ALLOCATED            0x8510007a
#define STORM_ERROR_MEM_CORRUPTED                0x8510007b
#define STORM_ERROR_MEM_INVALID                  0x8510007c
#define STORM_ERROR_MEM_MANAGER_NOT_INITIALIZED  0x8510007d
#define STORM_ERROR_MEM_NOT_FREED                0x8510007e
#define STORM_ERROR_RESOURCES_NOT_RELEASED       0x8510007f
#define STORM_ERROR_OUT_OF_BOUNDS                0x85100080
#define STORM_ERROR_NULL_POINTER                 0x85100081
#define STORM_ERROR_CDKEY_MISMATCH               0x85100082
#define STORM_ERROR_FILE_CORRUPTED               0x85100083
#define STORM_ERROR_FATAL                        0x85100084
#define STORM_ERROR_GAMETYPE_UNAVAILABLE         0x85100085

#define SMCopy(d,s) ( SMemCopy(d, s, __STORM_SSIZEMIN(s,d)) )
#define SMFill(l,f) (SMemFill(l, sizeof(l), f))
#define SMZero(l) (SMemZero(l, sizeof(l)))
#define SMCmp(l,x) ( SMemCmp(l, x, __STORM_SSIZEMIN(x,l)) )
#define SSCopy(d,s) (SStrCopy(d, s, sizeof(d)))

#define STORM_HASH_ABSOLUTE 1
#define SSCmp(s,x) ( SStrCmp(s,x,__STORM_SSIZEMIN(s,x)) )
#define SSCmpI(s,x) ( SStrCmpI(s,x,__STORM_SSIZEMIN(s,x)) )

BOOL  _SNetReceiveMessage(int *senderplayerid, char **data, int *databytes);
BOOL  _SNetSendMessage(int playerID, void *data, unsigned int databytes);
BOOL  _SNetReceiveTurns(int a1, int arraysize, char **arraydata, DWORD *arraydatabytes, DWORD *arrayplayerstatus);
BOOL  _SNetSendTurn(char *data, unsigned int databytes);
int  _SNetGetProviderCaps(struct _SNETCAPS *caps);
BOOL  _SNetUnregisterEventHandler(int evtype, SEVTHANDLER func);
BOOL  _SNetRegisterEventHandler(int evtype, SEVTHANDLER func);
BOOL  _SNetDestroy();
BOOL  _SNetDropPlayer(int playerid, DWORD flags);
BOOL  _SNetGetGameInfo(int type, void *dst, unsigned int length, unsigned int *byteswritten);
BOOL  _SNetLeaveGame(int type);
BOOL  _SNetSendServerChatCommand(const char *command);
int  _SNetInitializeProvider(unsigned long provider, struct _SNETPROGRAMDATA *client_info,
                           struct _SNETPLAYERDATA *user_info, struct _SNETUIDATA *ui_info,
                           struct _SNETVERSIONDATA *fileinfo);
BOOL  _SNetCreateGame(const char *pszGameName, const char *pszGamePassword, const char *pszGameStatString,
                      DWORD dwGameType, const char *GameTemplateData, int GameTemplateSize, int playerCount,
                      const char *creatorName, const char *a11, int *playerID);

BOOL  _SNetJoinGame(int id, char *pszGameName, char *pszGamePassword, char *playerName, char *userStats, int *playerID);
BOOL  _SNetGetOwnerTurnsWaiting(DWORD *turns);
BOOL  _SNetGetTurnsInTransit(int *turns);
BOOLEAN  _SNetSetBasePlayer(int);
BOOL  _SNetPerformUpgrade(DWORD *upgradestatus);
BOOL  _SNetSetGameMode(DWORD modeFlags, bool makePublic);

void*  SMemAlloc(unsigned int amount, const char *logfilename, int logline, int defaultValue);
BOOL  SMemFree(void *location, const char *logfilename, int logline, char defaultValue);

DWORD  SErrGetLastError();
void  SErrSetLastError(DWORD dwErrCode);

int  SStrCopy(char *dest, const char *src, int max_length);

BOOL  SRegLoadString(const char *keyname, const char *valuename, BYTE flags, char *buffer, unsigned int buffersize);
BOOL  SRegLoadValue(const char *keyname, const char *valuename, BYTE flags, int *value);
BOOL  SRegSaveString(const char *keyname, const char *valuename, BYTE flags, char *string);
BOOL  SRegSaveValue(const char *keyname, const char *valuename, BYTE flags, DWORD result);
BOOL  SRegDeleteValue(const char *keyname, const char *valuename, BYTE flags);

BOOL  SFileCloseArchive(HANDLE hArchive);
BOOL  SFileCloseFile(HANDLE hFile);
BOOL  SFileGetFileArchive(HANDLE hFile, HANDLE *archive);
LONG  SFileGetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
BOOL  SFileOpenArchive(const char *szMpqName, DWORD dwPriority, DWORD dwFlags, HANDLE *phMpq);
BOOL  SFileOpenFile(const char *filename, HANDLE *phFile);
BOOL  SFileOpenFileEx(HANDLE hMpq, const char *szFileName, DWORD dwSearchScope, HANDLE *phFile);
BOOL  SFileReadFile(HANDLE hFile, void *buffer, DWORD nNumberOfBytesToRead, DWORD *read, LONG *lpDistanceToMoveHigh);
BOOL  SFileSetBasePath(char *);
int  SFileSetFilePointer(HANDLE, int, HANDLE, int);
BOOL  SFileGetFileSizeFast(const char* filename, LPDWORD size);

BOOL  SBmpLoadImage(const char* pszFileName, PALETTEENTRY* pPalette, BYTE* pBuffer, DWORD dwBuffersize, DWORD* pdwWidth, DWORD* pdwHeight, DWORD* pdwBpp);
