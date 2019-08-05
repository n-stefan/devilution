#include "diablo.h"
#include "storm/storm.h"
#include "rmpq/file.h"

DWORD sgdwMpqOffset;
char mpq_buf[4096];
_HASHENTRY *sgpHashTbl;
BOOL save_archive_modified;
_BLOCKENTRY *sgpBlockTbl;
BOOLEAN save_archive_open;

//note: 32872 = 32768 + 104 (sizeof(_FILEHEADER))

/* data */

File sghArchive;

void mpqapi_xor_buf(char *pbData)
{
	DWORD mask;
	char *pbCurrentData;
	int i;

	mask = 0xF0761AB;
	pbCurrentData = pbData;

	for (i = 0; i < 8; i++) {
		*pbCurrentData ^= mask;
		pbCurrentData++;
    mask = (mask << 1) | (mask >> 31);
	}
}

void mpqapi_remove_hash_entry(const char *pszName)
{
	_HASHENTRY *pHashTbl;
	_BLOCKENTRY *blockEntry;
	int hIdx, block_offset, block_size;

	hIdx = FetchHandle(pszName);
	if (hIdx != -1) {
		pHashTbl = &sgpHashTbl[hIdx];
		blockEntry = &sgpBlockTbl[pHashTbl->block];
		pHashTbl->block = -2;
		block_offset = blockEntry->offset;
		block_size = blockEntry->sizealloc;
		memset(blockEntry, 0, sizeof(*blockEntry));
		mpqapi_alloc_block(block_offset, block_size);
		save_archive_modified = 1;
	}
}

void mpqapi_alloc_block(int block_offset, int block_size)
{
	_BLOCKENTRY *block;
	int i;

	block = sgpBlockTbl;
	i = 2048;
	while (i-- != 0) {
		if (block->offset && !block->flags && !block->sizefile) {
			if (block->offset + block->sizealloc == block_offset) {
				block_offset = block->offset;
				block_size += block->sizealloc;
				memset(block, 0, sizeof(_BLOCKENTRY));
				mpqapi_alloc_block(block_offset, block_size);
				return;
			}
			if (block_offset + block_size == block->offset) {
				block_size += block->sizealloc;
				memset(block, 0, sizeof(_BLOCKENTRY));
				mpqapi_alloc_block(block_offset, block_size);
				return;
			}
		}
		block++;
	}
	if (block_offset + block_size > sgdwMpqOffset) {
		app_fatal("MPQ free list error");
	}
	if (block_offset + block_size == sgdwMpqOffset) {
		sgdwMpqOffset = block_offset;
	} else {
		block = mpqapi_new_block(NULL);
		block->offset = block_offset;
		block->sizealloc = block_size;
		block->sizefile = 0;
		block->flags = 0;
	}
}

_BLOCKENTRY *mpqapi_new_block(int *block_index)
{
	_BLOCKENTRY *blockEntry;
	DWORD i;

	blockEntry = sgpBlockTbl;

	i = 0;
	while (blockEntry->offset || blockEntry->sizealloc || blockEntry->flags || blockEntry->sizefile) {
		i++;
		blockEntry++;
		if (i >= 2048) {
			app_fatal("Out of free block entries");
			return 0;
		}
	}
	if (block_index)
		*block_index = i;

	return blockEntry;
}

int FetchHandle(const char *pszName)
{
	return mpqapi_get_hash_index(Hash(pszName, 0), Hash(pszName, 1), Hash(pszName, 2), 0);
}

int mpqapi_get_hash_index(short index, int hash_a, int hash_b, int locale)
{
	int idx, i;

	i = 2048;
	for (idx = index & 0x7FF; sgpHashTbl[idx].block != -1; idx = (idx + 1) & 0x7FF) {
		if (!i--)
			break;
		if (sgpHashTbl[idx].hashcheck[0] == hash_a && sgpHashTbl[idx].hashcheck[1] == hash_b && sgpHashTbl[idx].lcid == locale && sgpHashTbl[idx].block != -2)
			return idx;
	}

	return -1;
}

void mpqapi_remove_hash_entries(BOOL( *fnGetName)(DWORD, char *))
{
	DWORD dwIndex, i;
	char pszFileName[MAX_PATH];

	dwIndex = 1;
	for (i = fnGetName(0, pszFileName); i; i = fnGetName(dwIndex++, pszFileName)) {
		mpqapi_remove_hash_entry(pszFileName);
	}
}

BOOL mpqapi_write_file(const char *pszName, const BYTE *pbData, DWORD dwLen)
{
	_BLOCKENTRY *blockEntry;

	save_archive_modified = TRUE;
	mpqapi_remove_hash_entry(pszName);
	blockEntry = mpqapi_add_file(pszName, 0, 0);
	if (!mpqapi_write_file_contents(pszName, pbData, dwLen, blockEntry)) {
		mpqapi_remove_hash_entry(pszName);
		return FALSE;
	}
	return TRUE;
}

_BLOCKENTRY *mpqapi_add_file(const char *pszName, _BLOCKENTRY *pBlk, int block_index)
{
	DWORD h1, h2, h3;
	int i, hIdx;

	h1 = Hash(pszName, 0);
	h2 = Hash(pszName, 1);
	h3 = Hash(pszName, 2);
	if (mpqapi_get_hash_index(h1, h2, h3, 0) != -1)
		app_fatal("Hash collision between \"%s\" and existing file\n", pszName);
	i = 2048;
	hIdx = h1 & 0x7FF;
	while (1) {
		i--;
		if (sgpHashTbl[hIdx].block == -1 || sgpHashTbl[hIdx].block == -2)
			break;
		hIdx = (hIdx + 1) & 0x7FF;
		if (!i) {
			i = -1;
			break;
		}
	}
	if (i < 0)
		app_fatal("Out of hash space");
	if (!pBlk)
		pBlk = mpqapi_new_block(&block_index);

	sgpHashTbl[hIdx].hashcheck[0] = h2;
	sgpHashTbl[hIdx].hashcheck[1] = h3;
	sgpHashTbl[hIdx].lcid = 0;
	sgpHashTbl[hIdx].block = block_index;

	return pBlk;
}

BOOL mpqapi_write_file_contents(const char *pszName, const BYTE *pbData, DWORD dwLen, _BLOCKENTRY *pBlk)
{
  std::vector<DWORD> sectoroffsettable;
	DWORD destsize, num_bytes, block_size, nNumberOfBytesToWrite;
	const BYTE *src;
	const char *tmp, *str_ptr;
	int i, j;

	str_ptr = pszName;
	src = pbData;
	while ((tmp = strchr(str_ptr, ':'))) {
		str_ptr = tmp + 1;
	}
	while ((tmp = strchr(str_ptr, '\\'))) {
		str_ptr = tmp + 1;
	}
	Hash(str_ptr, 3);
	num_bytes = (dwLen + 4095) >> 12;
	nNumberOfBytesToWrite = 4 * num_bytes + 4;
	pBlk->offset = mpqapi_find_free_block(dwLen + nNumberOfBytesToWrite, &pBlk->sizealloc);
	pBlk->sizefile = dwLen;
	pBlk->flags = 0x80000100;
  sghArchive.seek(pBlk->offset, SEEK_SET);
	j = 0;
	destsize = 0;
	while (dwLen != 0) {
		DWORD len;
		for (i = 0; i < 4096; i++)
			mpq_buf[i] -= 86;
		len = dwLen;
		if (dwLen >= 4096)
			len = 4096;
		memcpy(mpq_buf, src, len);
		src += len;
		len = PkwareCompress(mpq_buf, len);
		if (j == 0) {
			nNumberOfBytesToWrite = 4 * num_bytes + 4;
      sectoroffsettable.resize(num_bytes + 1, 0);
      if (sghArchive.write(sectoroffsettable.data(), nNumberOfBytesToWrite) != nNumberOfBytesToWrite) {
        return FALSE;
			}
			destsize += nNumberOfBytesToWrite;
		}
		sectoroffsettable[j] = destsize;
		if (sghArchive.write(mpq_buf, len) != len) {
      return FALSE;
		}
		j++;
		if (dwLen > 4096)
			dwLen -= 4096;
		else
			dwLen = 0;
		destsize += len;
	}

	sectoroffsettable[j] = destsize;
  sghArchive.seek(-destsize, SEEK_CUR);

  if (sghArchive.write(sectoroffsettable.data(), nNumberOfBytesToWrite) != nNumberOfBytesToWrite) {
    return FALSE;
  }

  sghArchive.seek(destsize - nNumberOfBytesToWrite, SEEK_CUR);

	if (destsize < pBlk->sizealloc) {
		block_size = pBlk->sizealloc - destsize;
		if (block_size >= 1024) {
			pBlk->sizealloc = destsize;
			mpqapi_alloc_block(pBlk->sizealloc + pBlk->offset, block_size);
		}
	}
	return TRUE;
}

int mpqapi_find_free_block(int size, int *block_size)
{
	_BLOCKENTRY *pBlockTbl;
	int i, result;

	pBlockTbl = sgpBlockTbl;
	i = 2048;
	while (1) {
		i--;
		if (pBlockTbl->offset && !pBlockTbl->flags && !pBlockTbl->sizefile && (DWORD)pBlockTbl->sizealloc >= size)
			break;
		pBlockTbl++;
		if (!i) {
			*block_size = size;
			result = sgdwMpqOffset;
			sgdwMpqOffset += size;
			return result;
		}
	}

	result = pBlockTbl->offset;
	*block_size = size;
	pBlockTbl->offset += size;
	pBlockTbl->sizealloc -= size;

	if (!pBlockTbl->sizealloc)
		memset(pBlockTbl, 0, sizeof(*pBlockTbl));

	return result;
}

void mpqapi_rename(char *pszOld, char *pszNew)
{
	int index, block;
	_HASHENTRY *hashEntry;
	_BLOCKENTRY *blockEntry;

	index = FetchHandle(pszOld);
	if (index != -1) {
		hashEntry = &sgpHashTbl[index];
		block = hashEntry->block;
		blockEntry = &sgpBlockTbl[block];
		hashEntry->block = -2;
		mpqapi_add_file(pszNew, blockEntry, block);
		save_archive_modified = TRUE;
	}
}

BOOL mpqapi_has_file(const char *pszName)
{
	return FetchHandle(pszName) != -1;
}

BOOL OpenMPQ(const char *pszArchive, BOOL hidden, DWORD dwChar)
{
	DWORD dwFlagsAndAttributes;
	DWORD key;
	_FILEHEADER fhdr;

	InitHash();
	dwFlagsAndAttributes = gbMaxPlayers > 1 ? 0 : 0;
	save_archive_open = FALSE;
  sghArchive = File(pszArchive, "rb+");
	if (!sghArchive) {
    sghArchive = File(pszArchive, "wb+");
		if (!sghArchive)
			return FALSE;
		save_archive_open = TRUE;
		save_archive_modified = TRUE;
	}
	if (sgpBlockTbl == NULL || sgpHashTbl == NULL) {
		memset(&fhdr, 0, sizeof(fhdr));
		if (ParseMPQHeader(&fhdr, &sgdwMpqOffset) == FALSE) {
      sghArchive = File();
      return FALSE;
    }
		sgpBlockTbl = (_BLOCKENTRY *)DiabloAllocPtr(0x8000);
		memset(sgpBlockTbl, 0, 0x8000);
		if (fhdr.blockcount) {
      sghArchive.seek(104, SEEK_SET);
      if (sghArchive.read(sgpBlockTbl, 0x8000) != 0x8000) {
        sghArchive = File();
        return FALSE;
      }
			key = Hash("(block table)", 3);
			Decrypt(sgpBlockTbl, 0x8000, key);
		}
		sgpHashTbl = (_HASHENTRY *)DiabloAllocPtr(0x8000);
		memset(sgpHashTbl, 255, 0x8000);
    if (fhdr.hashcount) {
      sghArchive.seek(32872, SEEK_SET);
      if (sghArchive.read(sgpHashTbl, 0x8000) != 0x8000) {
        sghArchive = File();
        return FALSE;
      }
			key = Hash("(hash table)", 3);
			Decrypt(sgpHashTbl, 0x8000, key);
		}
		return TRUE;
	}
	return TRUE;
}

BOOL ParseMPQHeader(_FILEHEADER *pHdr, DWORD *pdwNextFileStart)
{
	DWORD size;

  size = sghArchive.size();
	*pdwNextFileStart = size;

	if (size == -1
	    || size < sizeof(*pHdr)
	    || (sghArchive.read(pHdr, sizeof(*pHdr)) != 104)
	    || pHdr->signature != '\x1AQPM'
	    || pHdr->headersize != 32
	    || pHdr->version > 0
	    || pHdr->sectorsizeid != 3
	    || pHdr->filesize != size
	    || pHdr->hashoffset != 32872
	    || pHdr->blockoffset != 104
	    || pHdr->hashcount != 2048
	    || pHdr->blockcount != 2048) {

    sghArchive.seek(0, SEEK_SET);
		//if (SetFilePointer(sghArchive, 0, NULL, FILE_BEGIN) == -1)
		//	return FALSE;
		//if (!SetEndOfFile(sghArchive))
		//	return FALSE;

		memset(pHdr, 0, sizeof(*pHdr));
		pHdr->signature = '\x1AQPM';
		pHdr->headersize = 32;
		pHdr->sectorsizeid = 3;
		pHdr->version = 0;
		*pdwNextFileStart = 0x10068;
		save_archive_modified = TRUE;
		save_archive_open = 1;
	}

	return TRUE;
}

void CloseMPQ(const char *pszArchive, BOOL bFree, DWORD dwChar)
{
	if (bFree) {
		MemFreeDbg(sgpBlockTbl);
		MemFreeDbg(sgpHashTbl);
	}
	if (sghArchive) {
    sghArchive = File();
	}
	if (save_archive_modified) {
		save_archive_modified = FALSE;
	}
	if (save_archive_open) {
		save_archive_open = FALSE;
	}
}

BOOL mpqapi_flush_and_close(const char *pszArchive, BOOL bFree, DWORD dwChar)
{
	BOOL ret = FALSE;
	if (!sghArchive)
		ret = TRUE;
	else {
		ret = FALSE;
		if (!save_archive_modified)
			ret = TRUE;
		else if (mpqapi_can_seek() && WriteMPQHeader() && mpqapi_write_block_table()) {
			if (mpqapi_write_hash_table())
				ret = TRUE;
			else
				ret = FALSE;
		}
	}
	CloseMPQ(pszArchive, bFree, dwChar);
	return ret;
}

BOOL WriteMPQHeader()
{
	_FILEHEADER fhdr;

	memset(&fhdr, 0, sizeof(fhdr));
	fhdr.signature = '\x1AQPM';
	fhdr.headersize = 32;
	fhdr.filesize = sghArchive.size();
	fhdr.version = 0;
	fhdr.sectorsizeid = 3;
	fhdr.hashoffset = 32872;
	fhdr.blockoffset = 104;
	fhdr.hashcount = 2048;
	fhdr.blockcount = 2048;

  sghArchive.seek(0, SEEK_SET);
	if (sghArchive.write(&fhdr, sizeof(fhdr)) != 104)
		return 0;

	return TRUE;
}

BOOL mpqapi_write_block_table()
{
	DWORD NumberOfBytesWritten;

  sghArchive.seek(104, SEEK_SET);

	Encrypt(sgpBlockTbl, 0x8000, Hash("(block table)", 3));
  NumberOfBytesWritten = sghArchive.write(sgpBlockTbl, 0x8000);
	Decrypt(sgpBlockTbl, 0x8000, Hash("(block table)", 3));
	return NumberOfBytesWritten == 0x8000;
}

BOOL mpqapi_write_hash_table()
{
	DWORD NumberOfBytesWritten;

  sghArchive.seek(32872, SEEK_SET);

	Encrypt(sgpHashTbl, 0x8000, Hash("(hash table)", 3));
  NumberOfBytesWritten = sghArchive.write(sgpHashTbl, 0x8000);
	Decrypt(sgpHashTbl, 0x8000, Hash("(hash table)", 3));
	return NumberOfBytesWritten == 0x8000;
}

BOOL mpqapi_can_seek()
{
  sghArchive.seek(sgdwMpqOffset, SEEK_SET);
  sghArchive.truncate();
  return TRUE;
}
