#include "diablo.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>

EM_JS( void, exit_error, (const char* err), {
  var end = HEAPU8.indexOf( 0, err );
  var text = String.fromCharCode.apply(null, HEAPU8.subarray( err, end ));
  window.DX_ExitError( text );
});
EM_JS( void, show_alert, ( const char* err ), {
  var end = HEAPU8.indexOf( 0, err );
  var text = String.fromCharCode.apply( null, HEAPU8.subarray( err, end ) );
  window.alert( text );
});
#else
void exit_error( const char* err )
{
  if ( err )
  {
    MessageBox( NULL, err, "ERROR", MB_TASKMODAL | MB_ICONHAND );
  }
  exit( 1 );
}
void show_alert( const char* err )
{
  MessageBox( NULL, err, "Diablo", MB_TASKMODAL | MB_ICONEXCLAMATION );
}
#endif

char sz_error_buf[256];
BOOL terminating;
int cleanup_thread_id;

void TriggerBreak()
{
#ifdef _DEBUG
  __debugbreak();
#endif
}

char *GetErrorStr( DWORD error_code )
{
  sprintf( sz_error_buf, "unknown error 0x%08x", error_code );
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
    exit_error( Text );
  }
  else
  {
    va_end( va );
    exit_error( NULL );
  }

  init_cleanup( FALSE );
}

void __cdecl DrawDlg( char *pszFmt, ... )
{
  char text[256];
  va_list arglist;

  va_start( arglist, pszFmt );
  wvsprintf( text, pszFmt, arglist );
  va_end( arglist );

  show_alert( text );
}

#ifdef _DEBUG
void assert_fail( int nLineNo, const char *pszFile, const char *pszFail )
{
  app_fatal( "assertion failed (%d:%s)\n%s", nLineNo, pszFile, pszFail );
}
#endif

void ErrDlg( int template_id, DWORD error_code, char *log_file_path, int log_line_nr )
{
  app_fatal( "%s\nat: %s line %d", GetErrorStr( error_code ), log_file_path, log_line_nr );
}

void TextDlg( HWND hDlg, char *text )
{
  show_alert( text );
}

void ErrOkDlg( int template_id, DWORD error_code, char *log_file_path, int log_line_nr )
{
  char text[256];
  sprintf( text, "%s\nat: %s line %d", GetErrorStr( error_code ), log_file_path, log_line_nr );
  show_alert( text );
}

void FileErrDlg( const char *error )
{
  app_fatal( "%s", error );
}

void DiskFreeDlg( char *error )
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
