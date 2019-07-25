// temporary file

#ifndef _TYPES_H
#define _TYPES_H

//#define NO_WINDOWS

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#ifdef NO_WINDOWS

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

typedef int HWND;

typedef struct _FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

#else

#define WIN32_LEAN_AND_MEAN

#include "resource.h"

#include <windows.h>
#include <mmsystem.h>
//#include <ddraw.h>
#include <dsound.h>
#include <io.h>
#include <process.h>
#include <shlobj.h>
#include <shellapi.h>

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
