// temporary file

#ifndef _TYPES_H
#define _TYPES_H

#ifndef _WINDOWS_
#define NO_WINDOWS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#ifdef NO_WINDOWS

#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#pragma pack(push, 1)

  typedef unsigned long DWORD;
  typedef int BOOL;
  typedef unsigned char BYTE;
  typedef unsigned short WORD;
  typedef float FLOAT;
  typedef FLOAT *PFLOAT;
  typedef BOOL *PBOOL;
  typedef BOOL *LPBOOL;
  typedef BYTE *PBYTE;
  typedef BYTE *LPBYTE;
  typedef int *PINT;
  typedef int *LPINT;
  typedef WORD *PWORD;
  typedef WORD *LPWORD;
  typedef long *LPLONG;
  typedef DWORD *PDWORD;
  typedef DWORD *LPDWORD;
  typedef void *LPVOID;
  typedef const void *LPCVOID;

  typedef char CHAR;
  typedef short SHORT;
  typedef long LONG;
  typedef int INT;
  typedef unsigned int UINT;
  typedef unsigned int *PUINT;
  typedef BYTE BOOLEAN;
  typedef BOOLEAN *PBOOLEAN;

  typedef unsigned long ULONG;
  typedef ULONG *PULONG;
  typedef unsigned short USHORT;
  typedef USHORT *PUSHORT;
  typedef unsigned char UCHAR;
  typedef UCHAR *PUCHAR;
  typedef char *PSZ;

  typedef int INT_PTR, *PINT_PTR;
  typedef unsigned int UINT_PTR, *PUINT_PTR;
  typedef long LONG_PTR, *PLONG_PTR;
  typedef unsigned long ULONG_PTR, *PULONG_PTR;

  typedef UINT_PTR            WPARAM;
  typedef LONG_PTR            LPARAM;
  typedef LONG_PTR            LRESULT;

  typedef void* HANDLE;
  typedef LONG HRESULT;

  struct HWND__;
  typedef HWND__ *HWND;
  struct HINSTANCE__;
  typedef HINSTANCE__ *HINSTANCE;
  struct HDC__;
  typedef HDC__ *HDC;
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

  const LONG MAX_PATH = 260;

  typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
  } FILETIME, *PFILETIME, *LPFILETIME;

  typedef struct tWAVEFORMATEX {
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
                                    /* extra information (after cbSize) */
  } WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;

#define FALSE 0
#define TRUE 1

  typedef struct tagPALETTEENTRY {
    BYTE        peRed;
    BYTE        peGreen;
    BYTE        peBlue;
    BYTE        peFlags;
  } PALETTEENTRY, *PPALETTEENTRY, *LPPALETTEENTRY;

#define PC_RESERVED     0x01    /* palette index used for animation */
#define PC_EXPLICIT     0x02    /* palette index is explicit to device */
#define PC_NOCOLLAPSE   0x04    /* do not match color to system palette */

  typedef struct tagRECT {
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
  } RECT, *PRECT, *NPRECT, *LPRECT;

#define ERROR_FILE_NOT_FOUND             2L
#define ERROR_FILE_CORRUPT               1392L

  typedef struct IDirectSoundBuffer           *LPDIRECTSOUNDBUFFER;

  typedef DWORD           FOURCC;         /* a four character code */

  typedef struct _MMCKINFO {
    FOURCC          ckid;           /* chunk ID */
    DWORD           cksize;         /* chunk size */
    FOURCC          fccType;        /* form type or list type */
    DWORD           dwDataOffset;   /* offset of data portion of chunk */
    DWORD           dwFlags;        /* flags used by MMIO functions */
  } MMCKINFO, *PMMCKINFO, *NPMMCKINFO, *LPMMCKINFO;
  typedef const MMCKINFO *LPCMMCKINFO;

  typedef struct waveformat_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
  } WAVEFORMAT, *PWAVEFORMAT, *NPWAVEFORMAT, *LPWAVEFORMAT;
  typedef struct pcmwaveformat_tag {
    WAVEFORMAT  wf;
    WORD        wBitsPerSample;
  } PCMWAVEFORMAT, *PPCMWAVEFORMAT, *NPPCMWAVEFORMAT, *LPPCMWAVEFORMAT;

  typedef int(*GetFilesize)(const char* name);
  typedef void*(*GetFileContents)(const char* name);
  typedef void(*PutFileContents)(const char* name, void* ptr, size_t size);
  typedef void(*RemoveFile)(const char* name);
  typedef void(*SetCursor)(DWORD x, DWORD y);
  typedef void(*ExitGame)();
  typedef void(*ExitError)(const char* message);
  typedef void(*CurrentSaveId)(int id);

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#define mmioFOURCC(ch0, ch1, ch2, ch3)  MAKEFOURCC(ch0, ch1, ch2, ch3)
#define FOURCC_RIFF     mmioFOURCC('R', 'I', 'F', 'F')

#pragma pack(pop)

struct CCritSect {
  CCritSect() {
  }
  ~CCritSect() {
  }
  void Enter() {
  }
  void Leave() {
  }
};

#else

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
//#include <ddraw.h>
#include <dsound.h>
#include <io.h>
#include <process.h>
#include <shlobj.h>
#include <shellapi.h>

struct CCritSect {
  CRITICAL_SECTION m_critsect;

  CCritSect() {
    InitializeCriticalSection(&m_critsect);
  }
  ~CCritSect() {
    DeleteCriticalSection(&m_critsect);
  }
  void Enter() {
    EnterCriticalSection(&m_critsect);
  }
  void Leave() {
    LeaveCriticalSection(&m_critsect);
  }
};

#endif

#ifdef __GNUC__
#include <ctype.h>
#endif

// tell Visual C++ to shut the hell up
#ifdef _MSC_VER
#pragma warning(disable : 4305) // truncation of int
#pragma warning(disable : 4018) // signed/unsigned mismatch
#pragma warning(disable : 4700) // used without having been initialized
#pragma warning(disable : 4244) // conversion loss
#pragma warning(disable : 4146) // negative unsigned
#pragma warning(disable : 4996) // deprecation warning
#pragma warning(disable : 4309) // VC2017: truncation of constant value
#pragma warning(disable : 4267) // VC2017: conversion from 'size_t' to 'char'
#pragma warning(disable : 4302) // VC2017: type cast truncation
#pragma warning(disable : 4334) // VC2017: result of 32-bit shift implicitly converted to 64 bits
#endif

#include "defs.h"
#include "enums.h"
#include "structs.h"

#if (_MSC_VER >= 800) && (_MSC_VER <= 1200)
#define USE_ASM
#endif

// If defined, use copy protection [Default -> Defined]
#ifndef _DEBUG
#define COPYPROT
#endif

// If defined, don't reload for debuggers [Default -> Undefined]
// Note that with patch 1.03 the command line was hosed, this is required to pass arguments to the game
#ifdef _DEBUG
#define DEBUGGER
#endif

// If defined, don't fry the CPU [Default -> Undefined]
#ifdef _DEBUG
#define SLEEPFIX
#endif

// If defined, fix palette glitch in Windows Vista+ [Default -> Undefined]
//#define COLORFIX

#endif
