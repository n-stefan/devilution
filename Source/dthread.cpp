#include "diablo.h"
#include "storm/storm.h"

unsigned int glpDThreadId;
TMegaPkt *sgpInfoHead; /* may not be right struct */
BOOLEAN dthread_running;
HANDLE sghWorkToDoEvent;

DWORD netRemainingBytes = 0;
DWORD netLastCheckTime = 0;

DWORD& packetSize(TMegaPkt* pkt) {
  return *(DWORD *) &pkt->data[4];
}

/* rdata */
static HANDLE sghThread = INVALID_HANDLE_VALUE;

void dthread_remove_player(int pnum) {
  TMegaPkt *pkt;

  for (pkt = sgpInfoHead; pkt; pkt = pkt->pNext) {
    if (pkt->dwSpaceLeft == pnum)
      pkt->dwSpaceLeft = MAX_PLRS;
  }
}

void dthread_send_delta(int pnum, char cmd, void *pbSrc, int dwLen) {
  TMegaPkt *pkt;
  TMegaPkt *p;

  if (gbMaxPlayers == 1) {
    return;
  }

  pkt = (TMegaPkt *) DiabloAllocPtr(dwLen + 20);
  pkt->pNext = NULL;
  pkt->dwSpaceLeft = pnum;
  pkt->data[0] = cmd;
  packetSize(pkt) = dwLen;
  memcpy(&pkt->data[8], pbSrc, dwLen);
  p = (TMegaPkt *) &sgpInfoHead;
  while (p->pNext) {
    p = p->pNext;
  }
  p->pNext = pkt;
}

void dthread_start() {
  char *error_buf;

  if (gbMaxPlayers == 1) {
    return;
  }

  netRemainingBytes = gdwDeltaBytesSec;
  netLastCheckTime = _GetTickCount();
  dthread_running = TRUE;
}

void dthread_loop() {
  char *error_buf;
  TMegaPkt *pkt;
  DWORD dwMilliseconds;

  if (!dthread_running) {
    return;
  }

  netRemainingBytes += (_GetTickCount() - netLastCheckTime) * (gdwDeltaBytesSec / 1000);
  netLastCheckTime = _GetTickCount();
  if (netRemainingBytes > gdwDeltaBytesSec) {
    netRemainingBytes = gdwDeltaBytesSec;
  }

  while (sgpInfoHead && packetSize(pkt) <= netRemainingBytes) {
    pkt = sgpInfoHead;
    sgpInfoHead = sgpInfoHead->pNext;

    netRemainingBytes -= packetSize(pkt);

    if (pkt->dwSpaceLeft != MAX_PLRS)
      multi_send_zero_packet(pkt->dwSpaceLeft, pkt->data[0], &pkt->data[8], packetSize(pkt));

    mem_free_dbg(pkt);
  }
}

void dthread_cleanup() {
  char *error_buf;
  TMegaPkt *tmp;

  if (sghWorkToDoEvent == NULL) {
    return;
  }

  dthread_running = FALSE;

  while (sgpInfoHead) {
    tmp = sgpInfoHead->pNext;
    MemFreeDbg(sgpInfoHead);
    sgpInfoHead = tmp;
  }
}
