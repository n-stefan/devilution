#include <stdio.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include "diablo.h"

EM_JS( void, exit_error, (const char* err), {
  var end = HEAPU8.indexOf( 0, err );
  var text = String.fromCharCode.apply(null, HEAPU8.subarray( err, end ));
  self.DApi.exit_error( text );
});
EM_JS( void, show_alert, ( const char* err ), {
  var end = HEAPU8.indexOf( 0, err );
  var text = String.fromCharCode.apply( null, HEAPU8.subarray( err, end ) );
  self.alert( text );
});

void api_exit_error(const char* err) {
  exit_error(err);
  exit(1);
}

void api_show_alert(const char* err) {
  show_alert(err);
}

#else
#include <windows.h>
void api_exit_error(const char* err) {
  if (err) {
    ShowCursor(TRUE);
    MessageBox(NULL, err, "ERROR", MB_TASKMODAL | MB_ICONHAND);
  }
  __debugbreak();
  exit(1);
}
void api_show_alert(const char* err) {
  ShowCursor(TRUE);
  MessageBox(NULL, err, "Diablo", MB_TASKMODAL | MB_ICONEXCLAMATION);
}
#endif

#include "appfat.h"

char sz_error_buf[256];
BOOL terminating;
int cleanup_thread_id;

void TriggerBreak()
{
#ifdef _DEBUG
  //__debugbreak();
#endif
}

char *GetErrorStr( DWORD error_code )
{
  sprintf( sz_error_buf, "unknown error 0x%08x", (int) error_code );
  return sz_error_buf;
}

char *TraceLastError()
{
  return GetErrorStr( 0 );
}

void __cdecl app_fatal( const char *pszFmt, ... )
{
  va_list va;

  va_start( va, pszFmt );
#ifdef _DEBUG
  TriggerBreak();
#endif

  if ( pszFmt )
  {
    char Text[256];
    vsprintf( Text, pszFmt, va );
    va_end( va );
    api_exit_error( Text );
  }
  else
  {
    va_end( va );
    api_exit_error( NULL );
  }
}

void __cdecl DrawDlg( const char *pszFmt, ... )
{
  char text[256];
  va_list arglist;

  va_start( arglist, pszFmt );
  vsprintf( text, pszFmt, arglist );
  va_end( arglist );

  api_show_alert( text );
}

#ifdef _DEBUG
void assert_fail( int nLineNo, const char *pszFile, const char *pszFail )
{
  app_fatal( "assertion failed (%d:%s)\n%s", nLineNo, pszFile, pszFail );
}
#endif

void ErrDlg(int template_id, DWORD error_code, const char *log_file_path, int log_line_nr) {
  app_fatal("%s\nat: %s line %d", GetErrorStr(error_code), log_file_path, log_line_nr);
}
void ErrMsg(const char* text, const char *log_file_path, int log_line_nr) {
  app_fatal("%s\nat: %s line %d", text, log_file_path, log_line_nr);
}

void TextDlg( HWND hDlg, char *text )
{
  api_show_alert( text );
}

void ErrOkDlg( int template_id, DWORD error_code, const char *log_file_path, int log_line_nr )
{
  char text[256];
  sprintf( text, "%s\nat: %s line %d", GetErrorStr( error_code ), log_file_path, log_line_nr );
  api_show_alert( text );
}

void FileErrDlg( const char *error )
{
  app_fatal( "%s", error );
}

void DiskFreeDlg(const char *error )
{
  app_fatal( "%s", error );
}

BOOL InsertCDDlg()
{
  return TRUE;
}

void DirErrorDlg( char *error )
{
  app_fatal( "%s", error );
}
