#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include <string>

#define assert(x) sizeof(x)

enum PacketType : uint8_t {
  PT_MESSAGE = 0x01, // both
  PT_TURN = 0x02, // both
  PT_JOIN_ACCEPT = 0x12, // server
  PT_JOIN_REJECT = 0x15, // server
  PT_CONNECT = 0x13, // server
  PT_DISCONNECT = 0x14, // server

  PT_GAME_LIST = 0x21, // both
  PT_CREATE_GAME = 0x22, // client
  PT_JOIN_GAME = 0x23, // client
  PT_LEAVE_GAME = 0x24, // client
  PT_DROP_PLAYER = 0x03, // client
};

class parse_error : public std::exception {};

class buffer_reader {
public:
  buffer_reader(const uint8_t* data, size_t size)
    : ptr_(data)
    , end_(data + size)
  {
  }

  uint8_t read8() {
    if (ptr_ >= end_) {
      throw parse_error();
    }
    return *ptr_++;
  }

  uint8_t read16() {
    if (ptr_ + 2 > end_) {
      throw parse_error();
    }
    uint16_t result = uint16_t(ptr_[0]) + (uint16_t(ptr_[1]) << 8);
    ptr_ += 2;
    return result;
  }

  uint8_t read32() {
    if (ptr_ + 4 > end_) {
      throw parse_error();
    }
    uint16_t result = uint16_t(ptr_[0]) + (uint16_t(ptr_[1]) << 8) + (uint16_t(ptr_[2]) << 16) + (uint16_t(ptr_[3]) << 24);
    ptr_ += 2;
    return result;
  }

  std::string read_string() {
    size_t length = (size_t) read8();
    if (ptr_ + length > end_) {
      throw parse_error();
    }
    std::string result(ptr_, ptr_ + length);
    ptr_ += length;
    return result;
  }

  std::vector<uint8_t> rest() {
    std::vector<uint8_t> result(ptr_, end_);
    ptr_ = end_;
    return result;
  }

private:
  const uint8_t* ptr_;
  const uint8_t* end_;
};

class buffer_writer {
public:
  buffer_writer(uint8_t* ptr)
    : ptr_(ptr)
  {
  }

  void write8(uint8_t value) {
    *ptr_++ = value;
  }

  void write16(uint16_t value) {
    *ptr_++ = (uint8_t) value;
    *ptr_++ = (uint8_t)(value >> 8);
  }

  void write32(uint32_t value) {
    *ptr_++ = (uint8_t) value;
    *ptr_++ = (uint8_t)(value >> 8);
    *ptr_++ = (uint8_t)(value >> 16);
    *ptr_++ = (uint8_t)(value >> 24);
  }

  void write_string(const std::string& str) {
    *ptr_++ = (uint8_t) str.size();
    memcpy(ptr_, str.data(), str.size());
    ptr_ += str.size();
  }

  void write_vector(const std::vector<uint8_t>& vec) {
    memcpy(ptr_, vec.data(), vec.size());
    ptr_ += vec.size();
  }

private:
  uint8_t* ptr_;
};

class packet {
public:
  const uint8_t type;

  virtual size_t size() const = 0;
  virtual void serialize(uint8_t* ptr) const = 0;
};

class server_packet : public packet {
public:
  static std::unique_ptr<server_packet> deserialize(const uint8_t* data, size_t size);
};

template<PacketType PT>
class typed_server_packet : public server_packet {
public:
  typed_server_packet()
    : type(PT)
  {
  }
};

class server_game_list_packet : public typed_server_packet<PT_GAME_LIST> {
public:
  std::vector<std::pair<std::string, uint8_t>> games;

  server_game_list_packet() {}

  server_game_list_packet(buffer_reader& reader) {
    size_t count = reader.read8();
    while (count--) {
      uint8_t flags = reader.read8();
      games.emplace_back(reader.read_string(), flags);
    }
  }

  size_t size() const override {
    size_t result = 2;
    for (const auto& game : games) {
      result += 2 + game.first.size();
    }
    return result;
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write8(PT_GAME_LIST);
    writer.write8((uint8_t) games.size());
    for (const auto& game : games) {
      writer.write8(game.second);
      writer.write_string(game.first);
    }
  }
};

class server_join_accept_packet : public typed_server_packet<PT_JOIN_ACCEPT> {
public:
  std::vector<std::pair<std::string, uint8_t>> games;

  server_join_accept_packet() {}

  server_join_accept_packet(buffer_reader& reader) {
    size_t count = reader.read8();
    while (count--) {
      uint8_t flags = reader.read8();
      games.emplace_back(reader.read_string(), flags);
    }
  }

  size_t size() const override {
    size_t result = 2;
    for (const auto& game : games) {
      result += 2 + game.first.size();
    }
    return result;
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write8(PT_GAME_LIST);
    writer.write8((uint8_t) games.size());
    for (const auto& game : games) {
      writer.write8(game.second);
      writer.write_string(game.first);
    }
  }
};
