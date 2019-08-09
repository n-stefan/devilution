#pragma once

#include <stdint.h>
#include <array>
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

  PT_SERVER_INFO = 0x20, // server
  PT_GAME_LIST = 0x21, // both
  PT_CREATE_GAME = 0x22, // client
  PT_JOIN_GAME = 0x23, // client
  PT_LEAVE_GAME = 0x24, // client
  PT_DROP_PLAYER = 0x03, // client
};

typedef std::array<uint8_t, 8> init_info_t;

class parse_error : public std::exception {};

class buffer_reader {
public:
  buffer_reader(const uint8_t* data, size_t size)
    : ptr_(data)
    , end_(data + size)
  {
  }

  template<class T>
  T read() {
    if (ptr_ + sizeof(T) > end_) {
      throw parse_error();
    }
    T result;
    memcpy(&result, ptr_, sizeof(T));
    ptr_ += sizeof(T);
    return result;
  }

  template<class T>
  void read(T& result) {
    if (ptr_ + sizeof(T) > end_) {
      throw parse_error();
    }
    memcpy(&result, ptr_, sizeof(T));
    ptr_ += sizeof(T);
  }

  template<>
  std::string read<std::string>() {
    size_t length = read<uint8_t>();
    if (ptr_ + length > end_) {
      throw parse_error();
    }
    std::string result(ptr_, ptr_ + length);
    ptr_ += length;
    return result;
  }

  template<>
  void read(std::string& result) {
    size_t length = read<uint8_t>();
    if (ptr_ + length > end_) {
      throw parse_error();
    }
    result.assign(ptr_, ptr_ + length);
    ptr_ += length;
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

  template<class T>
  void write(const T& value) {
    memcpy(ptr_, &value, sizeof(T));
    ptr_ += sizeof(T);
  }

  template<class T>
  void write(const std::string& str) {
    *ptr_++ = (uint8_t) str.size();
    memcpy(ptr_, str.data(), str.size());
    ptr_ += str.size();
  }

  void rest(const std::vector<uint8_t>& vec) {
    memcpy(ptr_, vec.data(), vec.size());
    ptr_ += vec.size();
  }

private:
  uint8_t* ptr_;
};

class packet {
public:
  virtual size_t size() const = 0;
  virtual void serialize(uint8_t* ptr) const = 0;
};

class server_info_packet : public packet {
public:
  uint32_t version;

  server_info_packet(uint32_t version)
    : version(version)
  {
  }

  server_info_packet(buffer_reader& reader) {
    reader.read(version);
  }

  size_t size() const override {
    return 1 + sizeof(version);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_SERVER_INFO);
    writer.write(version);
  }
};

class server_game_list_packet : public packet {
public:
  std::vector<std::pair<std::string, uint8_t>> games;

  server_game_list_packet() {}

  server_game_list_packet(buffer_reader& reader) {
    size_t count = reader.read<uint8_t>();
    while (count--) {
      uint8_t flags = reader.read<uint8_t>();
      games.emplace_back(reader.read<std::string>(), flags);
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
    writer.write(PT_GAME_LIST);
    writer.write((uint8_t) games.size());
    for (const auto& game : games) {
      writer.write(game.second);
      writer.write(game.first);
    }
  }
};

class server_join_accept_packet : public packet {
public:
  uint32_t cookie;
  uint8_t index;
  init_info_t init_info;

  server_join_accept_packet(uint32_t cookie, uint8_t index, init_info_t init_info)
    : cookie(cookie)
    , index(index)
    , init_info(init_info)
  {
  }

  server_join_accept_packet(buffer_reader& reader) {
    reader.read(cookie);
    reader.read(index);
    reader.read(init_info);
  }

  size_t size() const override {
    return 1 + sizeof(cookie) + sizeof(index) + sizeof(init_info);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_JOIN_ACCEPT);
    writer.write(cookie);
    writer.write(index);
    writer.write(init_info);
  }
};

class server_join_reject_packet : public packet {
public:
  uint32_t cookie;
  uint8_t reason;

  server_join_reject_packet(uint32_t cookie, uint8_t reason)
    : cookie(cookie)
    , reason(reason)
  {
  }

  server_join_reject_packet(buffer_reader& reader) {
    reader.read(cookie);
    reader.read(reason);
  }

  size_t size() const override {
    return 1 + sizeof(cookie) + sizeof(reason);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_JOIN_REJECT);
    writer.write(cookie);
    writer.write(reason);
  }
};

class server_connect_packet : public packet {
public:
  uint8_t id;

  server_connect_packet(uint8_t id)
    : id(id)
  {
  }

  server_connect_packet(buffer_reader& reader) {
    reader.read(id);
  }

  size_t size() const override {
    return 1 + sizeof(id);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_CONNECT);
    writer.write(id);
  }
};

class server_disconnect_packet : public packet {
public:
  uint8_t id;
  uint32_t flags;

  server_disconnect_packet(uint8_t id, uint32_t flags)
    : id(id)
    , flags(flags)
  {
  }

  server_disconnect_packet(buffer_reader& reader) {
    reader.read(id);
    reader.read(flags);
  }

  size_t size() const override {
    return 1 + sizeof(id) + sizeof(flags);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_DISCONNECT);
    writer.write(id);
    writer.write(flags);
  }
};

class server_message_packet : public packet {
public:
  uint8_t id;
  std::vector<uint8_t> payload;

  server_message_packet(uint8_t id, std::vector<uint8_t>&& payload)
    : id(id)
    , payload(payload)
  {
  }

  server_message_packet(buffer_reader& reader) {
    reader.read(id);
    payload = reader.rest();
  }

  size_t size() const override {
    return 1 + sizeof(id) + payload.size();
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_MESSAGE);
    writer.write(id);
    writer.rest(payload);
  }
};

class server_turn_packet : public packet {
public:
  uint8_t id;
  std::vector<uint8_t> payload;

  server_turn_packet(uint8_t id, std::vector<uint8_t>&& payload)
    : id(id)
    , payload(payload) {
  }

  server_turn_packet(buffer_reader& reader) {
    reader.read(id);
    payload = reader.rest();
  }

  size_t size() const override {
    return 1 + sizeof(id) + payload.size();
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_TURN);
    writer.write(id);
    writer.rest(payload);
  }
};

class client_game_list_packet : public packet {
public:
  client_game_list_packet() {}
  client_game_list_packet(buffer_reader&) {}

  size_t size() const override {
    return 1;
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_GAME_LIST);
  }
};

class client_create_game_packet : public packet {
public:
  std::string name;
  std::string password;
  uint8_t flags;

  client_create_game_packet(std::string&& name, std::string&& password, uint8_t flags)
    : name(name)
    , password(password)
    , flags(flags)
  {
  }

  client_create_game_packet(buffer_reader& reader) {
    reader.read(name);
    reader.read(password);
    reader.read(flags);
  }

  size_t size() const override {
    return 3 + name.size() + password.size() + sizeof(flags);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_CREATE_GAME);
    writer.write(name);
    writer.write(password);
    writer.write(flags);
  }
};

class client_join_game_packet : public packet {
public:
  std::string name;
  std::string password;
  uint8_t flags;

  client_join_game_packet(std::string&& name, std::string&& password, uint8_t flags)
    : name(name)
    , password(password)
    , flags(flags)
  {
  }

  client_join_game_packet(buffer_reader& reader) {
    reader.read(name);
    reader.read(password);
    reader.read(flags);
  }

  size_t size() const override {
    return 3 + name.size() + password.size() + sizeof(flags);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_JOIN_GAME);
    writer.write(name);
    writer.write(password);
    writer.write(flags);
  }
};


class client_leave_game_packet : public packet {
public:
  client_leave_game_packet() {}
  client_leave_game_packet(buffer_reader&) {}

  size_t size() const override {
    return 1;
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_LEAVE_GAME);
  }
};

class client_message_packet : public packet {
public:
  uint8_t id;
  std::vector<uint8_t> payload;

  client_message_packet(uint8_t id, std::vector<uint8_t>&& payload)
    : id(id)
    , payload(payload)
  {
  }

  client_message_packet(buffer_reader& reader) {
    reader.read(id);
    payload = reader.rest();
  }

  size_t size() const override {
    return 1 + sizeof(id) + payload.size();
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_MESSAGE);
    writer.write(id);
    writer.rest(payload);
  }
};

class client_turn_packet : public packet {
public:
  uint8_t id;
  std::vector<uint8_t> payload;

  client_turn_packet(uint8_t id, std::vector<uint8_t>&& payload)
    : id(id)
    , payload(payload)
  {
  }

  client_turn_packet(buffer_reader& reader) {
    reader.read(id);
    payload = reader.rest();
  }

  size_t size() const override {
    return 1 + sizeof(id) + payload.size();
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_TURN);
    writer.write(id);
    writer.rest(payload);
  }
};

class client_drop_player_packet : public packet {
public:
  uint8_t id;
  uint32_t flags;

  client_drop_player_packet(uint8_t id, uint32_t flags)
    : id(id)
    , flags(flags)
  {
  }

  client_drop_player_packet(buffer_reader& reader) {
    reader.read(id);
    reader.read(flags);
  }

  size_t size() const override {
    return 1 + sizeof(id) + sizeof(flags);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(PT_DROP_PLAYER);
    writer.write(id);
    writer.write(flags);
  }
};
