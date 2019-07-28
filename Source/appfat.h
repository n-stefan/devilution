//HEADER_GOES_HERE
#ifndef __APPFAT_H__
#define __APPFAT_H__

extern char sz_error_buf[256];
extern int terminating;
extern int cleanup_thread_id;

void TriggerBreak();
char *GetErrorStr(DWORD error_code);
char *TraceLastError();
void __cdecl app_fatal(const char *pszFmt, ...);
void __cdecl DrawDlg(const char *pszFmt, ...);
#ifdef _DEBUG
void assert_fail(int nLineNo, const char *pszFile, const char *pszFail);
#endif
void ErrDlg(int template_id, DWORD error_code, const char *log_file_path, int log_line_nr);
void ErrMsg(const char* text, const char *log_file_path, int log_line_nr);
void TextDlg(HWND hDlg, char *text);
void ErrOkDlg(int template_id, DWORD error_code, const char *log_file_path, int log_line_nr);
void FileErrDlg(const char *error);
void DiskFreeDlg(const char *error);
BOOL InsertCDDlg();
void DirErrorDlg(const char *error);

#define DS_ERROR(code) ErrDlg(0, (code), __FILE__, __LINE__)
#define DX_ERROR(code) ErrDlg(0, (code), __FILE__, __LINE__)
#define ERROR_DLG(tmpl, code) ErrDlg((tmpl), (code), __FILE__, __LINE__)
#define ERROR_MSG(text) ErrMsg((text), __FILE__, __LINE__)

#endif /* __APPFAT_H__ */
