#define NOMINMAX
#include "storm.h"
#include "../rmpq/archive.h"

//BOOL SFileOpenFile(const char *filename, HANDLE *phFile) {
//  //eprintf("%s: %s\n", __FUNCTION__, filename);
//
//  bool result = false;
//
//  if (!result && patch_rt_mpq) {
//    result = SFileOpenFileEx((HANDLE) patch_rt_mpq, filename, 0, phFile);
//  }
//  if (!result) {
//    result = SFileOpenFileEx((HANDLE) diabdat_mpq, filename, 0, phFile);
//  }
//
//  if (!result || !*phFile) {
//    eprintf("%s: Not found: %s\n", __FUNCTION__, filename);
//  }
//  return result;
//}
//

#include <set>

BOOL  SFileOpenArchive(const char *szMpqName, DWORD dwPriority, DWORD dwFlags, HANDLE *phMpq) {
  File file(szMpqName, "rb");
  if (!file) {
    return FALSE;
  }
  mpq::Archive* archive = new mpq::Archive(file);
  *phMpq = (HANDLE) archive;
  return TRUE;
}

BOOL  SFileCloseArchive(HANDLE hArchive) {
  delete ((mpq::Archive*) hArchive);
  return TRUE;
}

struct MpqFile {
  File file;
  mpq::Archive* archive = nullptr;
};

BOOL  SFileCloseFile(HANDLE hFile) {
  delete ((MpqFile*) hFile);
  return TRUE;
}

BOOL  SFileGetFileArchive(HANDLE hFile, HANDLE *archive) {
  //MpqFile* file = (MpqFile*) hFile;
  *archive = NULL;
  return TRUE;
}

LONG  SFileGetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) {
  MpqFile* file = (MpqFile*) hFile;
  uint64_t size = file->file.size();
  if (lpFileSizeHigh) {
    *lpFileSizeHigh = (DWORD) (size >> 32);
  }
  return (LONG) size;
}

BOOL  SFileOpenFile(const char *filename, HANDLE *phFile) {
  bool result = false;
  if (!result && patch_rt_mpq) {
    result = SFileOpenFileEx((HANDLE) patch_rt_mpq, filename, 0, phFile);
  }
  if (!result) {
    result = SFileOpenFileEx((HANDLE) diabdat_mpq, filename, 0, phFile);
  }
  return result;
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
    MpqFile* mpqf = new MpqFile;
    mpqf->archive = archive;
    mpqf->file = file;
    *phFile = (HANDLE) mpqf;
    return TRUE;
  }
  SErrSetLastError(ERROR_FILE_CORRUPT);
  return FALSE;
}

BOOL  SFileReadFile(HANDLE hFile, void *buffer, DWORD nNumberOfBytesToRead, DWORD *read, LONG *lpDistanceToMoveHigh) {
  MpqFile* file = (MpqFile*) hFile;
  auto res = file->file.read(buffer, nNumberOfBytesToRead);
  if (read) {
    *read = (DWORD) res;
  }
  return res != 0;
}

BOOL  SFileSetBasePath(char *) {
  return TRUE;
}

int  SFileSetFilePointer(HANDLE hFile, int pos, HANDLE, int origin) {
  MpqFile* file = (MpqFile*) hFile;
  file->file.seek(pos, origin);
  return 0;
}

BOOL  SFileGetFileSizeFast(const char* filename, LPDWORD size) {
  if (patch_rt_mpq) {
    mpq::Archive* arc = (mpq::Archive*) patch_rt_mpq;
    auto pos = arc->findFile(filename);
    if (pos >= 0) {
      *size = arc->getFileSize(pos);
      return TRUE;
    }
  }
  if (diabdat_mpq) {
    mpq::Archive* arc = (mpq::Archive*) diabdat_mpq;
    auto pos = arc->findFile(filename);
    if (pos >= 0) {
      *size = arc->getFileSize(pos);
      return TRUE;
    }
  }
  return FALSE;
}
