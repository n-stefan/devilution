#include "../diablo.h"
#include "storm.h"

#include <memory>
#include <vector>
#include <deque>

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

DWORD SErrGetLastError() {
  return nLastError;
}

void SErrSetLastError(DWORD dwErrCode) {
  nLastError = dwErrCode;
}

int SStrCopy(char *dest, const char *src, int max_length) {
  strncpy(dest, src, max_length);
  return strlen(dest);
}
