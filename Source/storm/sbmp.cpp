#include "storm.h"

static int my_stricmp(const char* a, const char* b) {
  for (size_t i = 0; a[i] || b[i]; ++i) {
    char pa = tolower((unsigned char) a[i]);
    char pb = tolower((unsigned char) b[i]);
    if (pa != pb) {
      return pa < pb ? -1 : 1;
    }
  }
  return 0;
}

BOOL  SBmpLoadImage(const char* pszFileName, PALETTEENTRY* pPalette, BYTE* pBuffer, DWORD dwBuffersize, DWORD* pdwWidth, DWORD* pdwHeight, DWORD* pdwBpp) {
  HANDLE hFile;
  size_t size;
  PCXHEADER pcxhdr;
  BYTE paldata[256][3];
  BYTE *dataPtr, *fileBuffer;
  BYTE byte;

  if (pdwWidth)
    *pdwWidth = 0;
  if (pdwHeight)
    *pdwHeight = 0;
  if (pdwBpp)
    *pdwBpp = 0;

  if (!pszFileName || !*pszFileName) {
    return false;
  }

  if (pBuffer && !dwBuffersize) {
    return false;
  }

  if (!pPalette && !pBuffer && !pdwWidth && !pdwHeight) {
    return false;
  }

  if (!SFileOpenFile(pszFileName, &hFile)) {
    return false;
  }

  while (strchr(pszFileName, 92))
    pszFileName = strchr(pszFileName, 92) + 1;

  while (strchr(pszFileName + 1, 46))
    pszFileName = strchr(pszFileName, 46);

  // omit all types except PCX
  if (!pszFileName || my_stricmp(pszFileName, ".pcx")) {
    return false;
  }

  if (!SFileReadFile(hFile, &pcxhdr, 128, 0, 0)) {
    SFileCloseFile(hFile);
    return false;
  }

  int width = pcxhdr.Xmax - pcxhdr.Xmin + 1;
  int height = pcxhdr.Ymax - pcxhdr.Ymin + 1;

  if (pdwWidth)
    *pdwWidth = width;
  if (pdwHeight)
    *pdwHeight = height;
  if (pdwBpp)
    *pdwBpp = pcxhdr.BitsPerPixel;

  if (!pBuffer) {
    SFileSetFilePointer(hFile, 0, 0, 2);
    fileBuffer = NULL;
  } else {
    size = SFileGetFileSize(hFile, 0) - SFileSetFilePointer(hFile, 0, 0, 1);
    fileBuffer = (BYTE *) malloc(size);
  }

  if (fileBuffer) {
    SFileReadFile(hFile, fileBuffer, size, 0, 0);
    dataPtr = fileBuffer;

    for (int j = 0; j < height; j++) {
      for (int x = 0; x < width; dataPtr++) {
        byte = *dataPtr;
        if (byte < 0xC0) {
          *pBuffer = byte;
          pBuffer++;
          x++;
          continue;
        }
        dataPtr++;

        for (int i = 0; i < (byte & 0x3F); i++) {
          *pBuffer = *dataPtr;
          pBuffer++;
          x++;
        }
      }
    }

    free(fileBuffer);
  }

  if (pPalette && pcxhdr.BitsPerPixel == 8) {
    SFileSetFilePointer(hFile, -768, 0, 1);
    SFileReadFile(hFile, paldata, 768, 0, 0);
    for (int i = 0; i < 256; i++) {
      pPalette[i].peRed = paldata[i][0];
      pPalette[i].peGreen = paldata[i][1];
      pPalette[i].peBlue = paldata[i][2];
      pPalette[i].peFlags = 0;
    }
  }

  SFileCloseFile(hFile);

  return true;
}
