#include "common.h"

void UiDestroy() {
}
BOOL UiTitleDialog(int a1) {
  return TRUE;
}
void UiInitialize(void(__stdcall *fnSound)(char *file)) {
  gfnSoundFunction = fnSound;

  FontTables[AFT_SMALL] = LoadFileInMem("ui_art\\font16.bin", 0);
  FontTables[AFT_MED] = LoadFileInMem("ui_art\\font24.bin", 0);
  FontTables[AFT_BIG] = LoadFileInMem("ui_art\\font30.bin", 0);
  FontTables[AFT_HUGE] = LoadFileInMem("ui_art\\font42.bin", 0);
  LoadArtFont("ui_art\\font16s.pcx", AFT_SMALL, AFC_SILVER);
  LoadArtFont("ui_art\\font16g.pcx", AFT_SMALL, AFC_GOLD);
  LoadArtFont("ui_art\\font24s.pcx", AFT_MED, AFC_SILVER);
  LoadArtFont("ui_art\\font24g.pcx", AFT_MED, AFC_GOLD);
  LoadArtFont("ui_art\\font30s.pcx", AFT_BIG, AFC_SILVER);
  LoadArtFont("ui_art\\font30g.pcx", AFT_BIG, AFC_GOLD);
  LoadArtFont("ui_art\\font42g.pcx", AFT_HUGE, AFC_GOLD);

  LoadMaskedArtFont("ui_art\\smlogo.pcx", &ArtLogos[LOGO_MED], 15);
  LoadMaskedArtFont("ui_art\\focus16.pcx", &ArtFocus[FOCUS_SMALL], 8);
  LoadMaskedArtFont("ui_art\\focus.pcx", &ArtFocus[FOCUS_MED], 8);
  LoadMaskedArtFont("ui_art\\focus42.pcx", &ArtFocus[FOCUS_BIG], 8);
  LoadMaskedArtFont("ui_art\\cursor.pcx", &ArtCursor, 1, 0);
  LoadArt("ui_art\\heros.pcx", &ArtHero, 4);
  LoadBackgroundArt("ui_art\\title.pcx");
}
BOOL UiCopyProtError(int *pdwResult) {
  return TRUE;
}
void UiAppActivate(BOOL bActive) {
}
BOOL __fastcall UiValidPlayerName(char *name) {
  return TRUE;
}
BOOL UiSelHeroMultDialog(BOOL(__stdcall *fninfo)(BOOL(__stdcall *fninfofunc)(_uiheroinfo *)), BOOL(__stdcall *fncreate)(_uiheroinfo *), BOOL(__stdcall *fnremove)(_uiheroinfo *), BOOL(__stdcall *fnstats)(unsigned int, _uidefaultstats *), int *dlgresult, int *a6, char *name) {
  return TRUE;
}
BOOL UiSelHeroSingDialog(BOOL(__stdcall *fninfo)(BOOL(__stdcall *fninfofunc)(_uiheroinfo *)), BOOL(__stdcall *fncreate)(_uiheroinfo *), BOOL(__stdcall *fnremove)(_uiheroinfo *), BOOL(__stdcall *fnstats)(unsigned int, _uidefaultstats *), int *dlgresult, char *name, int *difficulty) {
  return TRUE;
}
BOOL UiCreditsDialog(int a1) {
  return TRUE;
}
BOOL UiMainMenuDialog(char *name, int *pdwResult, void(__stdcall *fnSound)(char *file), int a4) {
  return TRUE;
}
BOOL UiProgressDialog(HWND window, char *msg, int enable, int(*fnfunc)(), int rate) {
  return TRUE;
}
int UiProfileGetString() {
  return 0;
}
void __cdecl UiProfileCallback() {
}
void __cdecl UiProfileDraw() {
}
BOOL UiCategoryCallback(int a1, int a2, int a3, int a4, int a5, DWORD *a6, DWORD *a7) {
  return TRUE;
}
BOOL UiGetDataCallback(int game_type, int data_code, void *a3, int a4, int a5) {
  return TRUE;
}
BOOL UiAuthCallback(int a1, char *a2, char *a3, char a4, char *a5, LPSTR lpBuffer, int cchBufferMax) {
  return TRUE;
}
BOOL UiSoundCallback(int a1, int type, int a3) {
  return TRUE;
}
void UiMessageBoxCallback(HWND hWnd, char *lpText, LPCSTR lpCaption, UINT uType) {
}
BOOL UiDrawDescCallback(int arg0, COLORREF color, LPCSTR lpString, char *a4, int a5, UINT align, time_t a7, HDC *a8) {
  return TRUE;
}
BOOL UiCreateGameCallback(int a1, int a2, int a3, int a4, int a5, int a6) {
  return TRUE;
}
BOOL UiArtCallback(int game_type, unsigned int art_code, PALETTEENTRY *pPalette, BYTE *pBuffer, DWORD dwBuffersize, DWORD *pdwWidth, DWORD *pdwHeight, DWORD *pdwBpp) {
  return TRUE;
}
int UiSelectGame(int a1, _SNETPROGRAMDATA *client_info, _SNETPLAYERDATA *user_info, _SNETUIDATA *ui_info, _SNETVERSIONDATA *file_info, int *a6) {
  return 0;
}
int UiSelectProvider(int a1, _SNETPROGRAMDATA *client_info, _SNETPLAYERDATA *user_info, _SNETUIDATA *ui_info, _SNETVERSIONDATA *file_info, int *type) {
  return 0;
}
BOOL UiCreatePlayerDescription(_uiheroinfo *info, int mode, char *desc) {
  return TRUE;
}
void UiSetupPlayerInfo(char *infostr, _uiheroinfo *pInfo, int type) {
}
void UiCreateGameCriteria(_uiheroinfo *pInfo, char *str) {
}
BOOL UiGetDefaultStats(int pclass, _uidefaultstats *pStats) {
  return TRUE;
}
BOOL UiBetaDisclaimer(int a1) {
  return TRUE;
}
