#define NOMINMAX
#include "archive.h"
#include <algorithm>
#include "../diablo.h"
#include "../trace.h"

namespace mpq {

Archive::Archive(File file)
  : file_(file)
{
  size_t size = file.size();
  offset_ = 0;
  while (offset_ + sizeof(uint32_t) <= size) {
    file.seek(offset_);
    uint32_t id = file.read32();
    if (id == MPQHeader::signature) {
      break;
    } else if (id == MPQUserData::signature) {
      MPQUserData userdata;
      file.seek(offset_);
      if (file.read(&userdata, sizeof userdata) == sizeof userdata) {
        offset_ += userdata.headerOffs;
        break;
      }
    }
    offset_ += 512;
  }
  if (offset_ >= size) {
    ERROR_MSG("invalid MPQ file");
  }
  file.seek(offset_);

  if (file.read(&header_, MPQHeader::size_v1) != MPQHeader::size_v1 || header_.id != MPQHeader::signature) {
    ERROR_MSG("corrupted MPQ file");
  }

  if (header_.formatVersion > 1 && header_.headerSize > MPQHeader::size_v1) {
    if (file.read((uint8_t*)&header_ + MPQHeader::size_v1, header_.headerSize - MPQHeader::size_v1) != header_.headerSize - MPQHeader::size_v1) {
      ERROR_MSG("corrupted MPQ file");
    }
  }

  header_.upgrade(size - offset_);

  uint64_t hashTablePos = offset64(header_.hashTablePos, header_.hashTablePosHi) + offset_;
  uint64_t blockTablePos = offset64(header_.blockTablePos, header_.blockTablePosHi) + offset_;

  hashTable_.resize(header_.hashTableSize);
  file.seek(hashTablePos);
  if (file.read(hashTable_.data(), hashTable_.size() * sizeof(MPQHashEntry)) != hashTable_.size() * sizeof(MPQHashEntry)) {
    ERROR_MSG("corrupted MPQ file");
  }

  blockTable_.resize(header_.blockTableSize);
  file.seek(blockTablePos);
  if (file.read(blockTable_.data(), blockTable_.size() * sizeof(MPQBlockEntry)) != blockTable_.size() * sizeof(MPQBlockEntry)) {
    ERROR_MSG("corrupted MPQ file");
  }

  if (header_.hiBlockTablePos64) {
    hiBlockTable_.resize(header_.blockTableSize);
    if (file.read(hiBlockTable_.data(), hiBlockTable_.size() * sizeof(uint16_t)) != hiBlockTable_.size() * sizeof(uint16_t)) {
      ERROR_MSG("corrupted MPQ file");
    }
  }

  names_.resize(header_.hashTableSize);

  decryptBlock(hashTable_.data(), hashTable_.size() * sizeof(MPQHashEntry), hashString("(hash table)", HASH_KEY));
  decryptBlock(blockTable_.data(), blockTable_.size() * sizeof(MPQBlockEntry), hashString("(block table)", HASH_KEY));

  blockSize_ = (1 << (9 + header_.blockSize));
  if (blockSize_ > 0x200000) {
    blockSize_ = 0x100000;
    for (auto& bt : blockTable_) {
      while (bt.fSize > blockSize_) {
        blockSize_ *= 2;
      }
    }
  }

  buffer_.resize(blockSize_ * 2 + 64);

  quickTable_.reserve(blockTable_.size());
  unknowns_ = 0;
  for (size_t i = 0; i < hashTable_.size(); ++i) {
    auto& ht = hashTable_[i];
    if (ht.blockIndex < blockTable_.size()) {
      quickTable_.emplace(hashTo64(ht.name1, ht.name2), i);
      ++unknowns_;
    }
  }
}

void Archive::addName_(char const* name) {
  uint64_t hash = hashString64(name);
  auto it = quickTable_.find(hash);
  if (it == quickTable_.end()) {
    return;
  }

  if (names_[it->second].empty()) {
    if (unknowns_) unknowns_ -= 1;
  }
  names_[it->second] = name;
}

void Archive::loadListFile() {
  addName_("(listfile)");
  addName_("(attributes)");
  addName_("(signature)");
  addName_("(patch_metadata)");
  File list = load("(listfile)");
  listFiles(list);
}

std::string trim(std::string const& str) {
  size_t left = 0, right = str.size();
  while (left < str.length() && isspace((unsigned char) str[left])) ++left;
  while (right > left && isspace((unsigned char) str[right - 1])) --right;
  return str.substr(left, right - left);
}

void Archive::listFiles(File list) {
  if (!list) return;
  std::string line;
  while (list.getline(line)) {
    line = trim(line);
    if (line.length()) {
      addName_(line.c_str());
    }
  }
}

intptr_t Archive::findFile(char const* name) const {
  uint64_t hash = hashString64(name);
  auto it = quickTable_.find(hash);
  if (it == quickTable_.end()) {
    return -1;
  }
  if (names_[it->second].empty()) {
    if (unknowns_) unknowns_ -= 1;
    names_[it->second] = name;
  }
  return it->second;
}

intptr_t Archive::findFile(char const* name, uint16_t locale) const {
  uint32_t name1 = hashString(name, HASH_NAME1);
  uint32_t name2 = hashString(name, HASH_NAME2);
  size_t count = 0;

  intptr_t best = -1;
  for (size_t cur = hashString(name, HASH_OFFSET) % hashTable_.size();
    count < hashTable_.size() && hashTable_[cur].blockIndex != MPQHashEntry::EMPTY;
    count++, cur = (cur + 1) % hashTable_.size())
  {
    if (hashTable_[cur].blockIndex < blockTable_.size() &&
      hashTable_[cur].name1 == name1 && hashTable_[cur].name2 == name2)
    {
      if (names_[cur].empty()) {
        if (unknowns_) unknowns_ -= 1;
        names_[cur] = name;
      }
      if (hashTable_[cur].locale == locale) {
        return cur;
      }
      if (best < 0 || hashTable_[cur].locale == Locale::Neutral) {
        best = cur;
      }
    }
  }
  return best;
}

bool Archive::fileExists(char const* name) const {
  return findFile(name) >= 0;
}

bool Archive::fileExists(char const* name, uint16_t locale) const {
  auto pos = findFile(name, locale);
  return pos >= 0 && hashTable_[pos].locale == locale;
}

size_t Archive::getMaxFiles() const {
  return hashTable_.size();
}

bool Archive::fileExists(size_t pos) const {
  return pos <= hashTable_.size() && hashTable_[pos].blockIndex < blockTable_.size();
}

bool Archive::testFile(size_t pos) {
  if (!fileExists(pos)) {
    return false;
  }
  uint32_t block = hashTable_[pos].blockIndex;
  if (block >= blockTable_.size()) {
    return false;
  }
  uint64_t filePos = blockTable_[block].filePos;
  if (hiBlockTable_.size()) {
    filePos |= uint64_t(hiBlockTable_[block]) << 32;
  }
  size_t fileSize = blockTable_[block].fSize;
  size_t cmpSize = blockTable_[block].cSize;
  uint32_t flags = blockTable_[block].flags;

  if (!(flags & FileFlags::Encrypted)) {
    return true;
  }
  if (!names_[pos].empty()) {
    return true;
  }

  file_.seek(offset_ + filePos);
  if (flags & FileFlags::PatchFile) {
    uint32_t patchLength = file_.read32();
    file_.seek(patchLength - sizeof(uint32_t), SEEK_CUR);
  }
  if ((flags & FileFlags::SingleUnit) || !(flags & FileFlags::Compressed)) {
    uint32_t rSize = (cmpSize > 12 ? 12 : cmpSize);
    memset(buffer_.data(), 0, 12);
    if (file_.read(buffer_.data(), rSize) != rSize) {
      return false;
    }
    if (detectFileSeed((uint32_t*)buffer_.data(), fileSize) != 0) {
      return true;
    }
  } else {
    uint32_t tableSize = 4 * ((fileSize + blockSize_ - 1) / blockSize_);
    if (flags & FileFlags::SectorCrc) {
      tableSize += 4;
    }
    if (file_.read(buffer_.data(), 8) != 8) {
      return false;
    }
    if (detectTableSeed((uint32_t*)buffer_.data(), tableSize, blockSize_) != 0) {
      return true;
    }
  }
  return false;
}

size_t Archive::getFileSize(size_t pos) const {
  uint32_t block = hashTable_[pos].blockIndex;
  return blockTable_[block].fSize;
}

size_t Archive::getFileCSize(size_t pos) const {
  uint32_t block = hashTable_[pos].blockIndex;
  return blockTable_[block].cSize;
}

File Archive::load(char const* name) {
  auto pos = findFile(name);
  if (pos < 0) {
    return File();
  }
  return load_(pos, hashString(path_name(name), HASH_KEY), true);
}

File Archive::load(char const* name, uint16_t locale) {
  auto pos = findFile(name, locale);
  if (pos < 0) {
    return File();
  }
  return load_(pos, hashString(path_name(name), HASH_KEY), true);
}

File Archive::load(size_t index) {
  if (!fileExists(index)) {
    return File();
  }
  return load_(index, 0, false);
}

intptr_t Archive::findNextFile(char const* name, intptr_t from) const {
  uint32_t name1 = hashString(name, HASH_NAME1);
  uint32_t name2 = hashString(name, HASH_NAME2);
  size_t count = 0;

  size_t cur = (from < 0 ? hashString(name, HASH_OFFSET) : from + 1) % hashTable_.size();

  for (; count < hashTable_.size() && cur != from && hashTable_[cur].blockIndex != MPQHashEntry::EMPTY;
    count++, cur = (cur + 1) % hashTable_.size())
  {
    if (hashTable_[cur].blockIndex < blockTable_.size() && hashTable_[cur].name1 == name1 && hashTable_[cur].name2 == name2) {
      if (names_[cur].empty()) {
        if (unknowns_) unknowns_ -= 1;
        names_[cur] = name;
      }
      return cur;
    }
  }
  return -1;
}

char const* Archive::getFileName(size_t index) const {
  if (index < names_.size()) {
    return names_[index].empty() ? nullptr : names_[index].c_str();
  }
  return nullptr;
}

File Archive::load_(size_t pos, uint32_t key, bool keyValid) {
  uint32_t block = hashTable_[pos].blockIndex;
  uint64_t filePos = blockTable_[block].filePos;
  if (hiBlockTable_.size()) {
    filePos |= uint64_t(hiBlockTable_[block]) << 32;
  }
  size_t fileSize = blockTable_[block].fSize;
  size_t cmpSize = blockTable_[block].cSize;
  uint32_t flags = blockTable_[block].flags;

  if (flags & FileFlags::PatchFile) {
    return File();
  }

  std::vector<uint8_t> data(fileSize);

  if (!(flags & FileFlags::Compressed)) {
    cmpSize = fileSize;
  }

  if (keyValid && (flags & FileFlags::FixSeed)) {
    key = (key + uint32_t(filePos)) ^ fileSize;
  }

  if (flags & FileFlags::SingleUnit) {
    std::vector<uint8_t> temp;
    uint8_t* buf;
    if (flags & FileFlags::Compressed) {
      temp.resize(cmpSize);
      buf = temp.data();
    } else {
      buf = data.data();
    }
    file_.seek(offset_ + filePos);
    if (file_.read(buf, cmpSize) != cmpSize) {
      return File();
    }
    if (flags & FileFlags::Encrypted) {
      if (!keyValid) {
        key = detectFileSeed((uint32_t*)buf, fileSize);
        if (key) {
          keyValid = true;
        }
      }
      if (keyValid) {
        decryptBlock(buf, cmpSize, key);
      } else {
        return File();
      }
    }
    if (flags & FileFlags::CompressMulti) {
      size_t size = fileSize;
      if (!multi_decompress(buf, cmpSize, data.data(), &size) || size != fileSize) {
        return File();
      }
    } else if (flags & FileFlags::CompressPkWare) {
      size_t size = fileSize;
      if (!pkzip_decompress(buf, cmpSize, data.data(), &size) || size != fileSize) {
        return File();
      }
    }
  } else if (!(flags & FileFlags::Compressed)) {
    file_.seek(offset_ + filePos);
    if (file_.read(data.data(), fileSize) != fileSize) {
      return File();
    }
    if (flags & FileFlags::Encrypted) {
      if (!keyValid) {
        key = detectFileSeed((uint32_t*)data.data(), fileSize);
        if (key) {
          keyValid = true;
        }
      }
      if (keyValid) {
        for (size_t offs = 0; offs < fileSize; offs += blockSize_) {
          decryptBlock(data.data() + offs, std::min(fileSize - offs, blockSize_), key + offs / blockSize_);
        }
      } else {
        return File();
      }
    }
  } else {
    size_t numBlocks = (fileSize + blockSize_ - 1) / blockSize_;
    size_t tableSize = numBlocks + 1;
    if (flags & (FileFlags::SectorCrc)) {
      tableSize += 1;
    }
    std::vector<uint32_t> blocks(tableSize);
    file_.seek(offset_ + filePos);
    if (file_.read(blocks.data(), tableSize * sizeof(uint32_t)) != tableSize * sizeof(uint32_t)) {
      return File();
    }
    if (flags & FileFlags::Encrypted) {
      if (!keyValid) {
        key = detectTableSeed(blocks.data(), tableSize * sizeof(uint32_t), blockSize_);
        if (key) {
          keyValid = true;
        }
      }
      if (keyValid) {
        decryptBlock(blocks.data(), tableSize * sizeof(uint32_t), key - 1);
      } else {
        return File();
      }
    }

    uint8_t* sBuf = buffer_.data();
    uint8_t* dBuf = sBuf + blockSize_;

    for (size_t block = 0; block < numBlocks; ++block) {
      size_t oPos = block * blockSize_;
      size_t cSize = blocks[block + 1] - blocks[block];
      size_t uSize = std::min(blockSize_, fileSize - oPos);
      file_.seek(offset_ + filePos + blocks[block]);
      if (file_.read(sBuf, cSize) != cSize) {
        return File();
      }
      if (flags & FileFlags::Encrypted) {
        decryptBlock(sBuf, cSize, key + block);
      }
      if (flags & FileFlags::CompressMulti) {
        size_t size = uSize;
        if (!multi_decompress(sBuf, cSize, data.data() + oPos, &size, dBuf) || size != uSize) {
          return File();
        }
      } else if (flags & FileFlags::CompressPkWare) {
        size_t size = uSize;
        if (!pkzip_decompress(sBuf, cSize, data.data() + oPos, &size) || size != uSize) {
          return File();
        }
      }
    }
  }

  return MemoryFile(std::move(data));
}

}
