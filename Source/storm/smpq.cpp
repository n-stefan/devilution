#define NOMINMAX
#include "storm.h"
#include "../rmpq/archive.h"
#include "../trace.h"

#include <set>

BOOL  SFileOpenArchive(const char *szMpqName, DWORD dwPriority, DWORD dwFlags, HANDLE *phMpq) {
  File file(szMpqName, "rb");
  if (!file) {
    return FALSE;
  }
  mpq::Archive* archive = new mpq::Archive(file);
#ifndef EMSCRIPTEN
  //archive->listFiles(File("listfile.txt"));
  //File fo("sizes.txt", "wb");
  //for (size_t i = 0; i < archive->getMaxFiles(); ++i) {
  //  auto name = archive->getFileName(i);
  //  if (name) {
  //    std::string sname(name);
  //    strlwr(&sname[0]);
  //    fo.printf("%s\t%s\t%u\t%u\r\n", sname.c_str(), mpq::path_ext(sname.c_str()), archive->getFileCSize(i), archive->getFileSize(i));
  //  }
  //}
#endif
  *phMpq = (HANDLE) archive;
  return TRUE;
}

BOOL  SFileCloseArchive(HANDLE hArchive) {
  delete ((mpq::Archive*) hArchive);
  return TRUE;
}

BOOL  SFileCloseFile(HANDLE hFile) {
  delete ((File*) hFile);
  return TRUE;
}

BOOL  SFileGetFileArchive(HANDLE hFile, HANDLE *archive) {
  //MpqFile* file = (MpqFile*) hFile;
  *archive = NULL;
  return TRUE;
}

LONG  SFileGetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) {
  File* file = (File*) hFile;
  uint64_t size = file->size();
  if (lpFileSizeHigh) {
    *lpFileSizeHigh = (DWORD) (size >> 32);
  }
  return (LONG) size;
}

BOOL  SFileOpenFile(const char *filename, HANDLE *phFile) {
  return SFileOpenFileEx((HANDLE) diabdat_mpq, filename, 0, phFile);
}

BOOL  SFileOpenFileEx(HANDLE hMpq, const char *szFileName, DWORD dwSearchScope, HANDLE *phFile) {
  mpq::Archive* archive = (mpq::Archive*) hMpq;
  auto pos = archive->findFile(szFileName);
  if (pos < 0) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
  }
  auto file = archive->load_(pos, mpq::hashString(mpq::path_name(szFileName), mpq::HASH_KEY), true);
  if (file) {
    File* mpqf = new File(file);
    *phFile = (HANDLE) mpqf;
    return TRUE;
  }
  SErrSetLastError(ERROR_FILE_CORRUPT);
  return FALSE;
}

BOOL  SFileReadFile(HANDLE hFile, void *buffer, DWORD nNumberOfBytesToRead, DWORD *read, LONG *lpDistanceToMoveHigh) {
  File* file = (File*) hFile;
  auto res = file->read(buffer, nNumberOfBytesToRead);
  if (read) {
    *read = (DWORD) res;
  }
  return res != 0;
}

BOOL  SFileSetBasePath(char *) {
  return TRUE;
}

int  SFileSetFilePointer(HANDLE hFile, int pos, HANDLE, int origin) {
  File* file = (File*) hFile;
  file->seek(pos, origin);
  return 0;
}

BOOL  SFileGetFileSizeFast(const char* filename, LPDWORD size) {
  mpq::Archive* arc = (mpq::Archive*) diabdat_mpq;
  auto pos = arc->findFile(filename);
  if (pos >= 0) {
    *size = arc->getFileSize(pos);
    return TRUE;
  }
  return FALSE;
}
