#pragma once

#include <stdint.h>
#include <array>
#include <memory>
#include <vector>
#include <string>

//#define assert(x) sizeof(x)

enum PacketType : uint8_t {
  PT_MESSAGE = 0x01, // both
  PT_TURN = 0x02, // both
  PT_JOIN_ACCEPT = 0x12, // server
  PT_JOIN_REJECT = 0x15, // server
  PT_CONNECT = 0x13, // server
  PT_DISCONNECT = 0x14, // server

  PT_CLIENT_INFO = 0x31, // client
  PT_SERVER_INFO = 0x32, // server

  PT_GAME_LIST = 0x21, // both
  PT_CREATE_GAME = 0x22, // client
  PT_JOIN_GAME = 0x23, // client
  PT_LEAVE_GAME = 0x24, // client
  PT_DROP_PLAYER = 0x03, // client
};

enum RejectionReason : uint8_t {
  JOIN_SUCCESS = 0x00,
  JOIN_ALREADY_IN_GAME = 0x01,
  JOIN_GAME_NOT_FOUND = 0x02,
  JOIN_INCORRECT_PASSWORD = 0x03,
  JOIN_VERSION_MISMATCH = 0x04,
  JOIN_GAME_FULL = 0x05,

  CREATE_GAME_EXISTS = 0x06,
};

enum LeaveReason : uint32_t {
  LEAVE_NORMAL = 0x03u,
  LEAVE_ENDING = 0x40000004u,
  LEAVE_DROP = 0x40000006u,
};

class parse_error : public std::exception {};

class buffer_reader {
public:
  buffer_reader(const uint8_t* data, size_t size)
      : ptr_(data)
      , end_(data + size) {
  }

  bool done() const {
    return ptr_ == end_;
  }

  template<class T>
  typename std::enable_if<std::is_pod<T>::value>::type
  read(T& result) {
    if (ptr_ + sizeof(T) > end_) {
      throw parse_error();
    }
    memcpy(&result, ptr_, sizeof(T));
    ptr_ += sizeof(T);
  }

  void read(std::string& result) {
    size_t length = read<uint8_t>();
    if (ptr_ + length > end_) {
      throw parse_error();
    }
    result.assign(ptr_, ptr_ + length);
    ptr_ += length;
  }

  void read(std::vector<uint8_t>& result) {
    size_t length = read<uint32_t>();
    if (ptr_ + length > end_) {
      throw parse_error();
    }
    result.assign(ptr_, ptr_ + length);
    ptr_ += length;
  }

  template<class T>
  T read() {
    T result;
    read(result);
    return result;
  }

private:
  const uint8_t* ptr_;
  const uint8_t* end_;
};

class buffer_writer {
public:
  buffer_writer(uint8_t* ptr)
      : ptr_(ptr) {
  }

  template<class T>
  typename std::enable_if<std::is_pod<T>::value>::type
  write(const T& value) {
    memcpy(ptr_, &value, sizeof(T));
    ptr_ += sizeof(T);
  }

  void write(const std::string& str) {
    *ptr_++ = (uint8_t) str.size();
    memcpy(ptr_, str.data(), str.size());
    ptr_ += str.size();
  }

  void write(const std::vector<uint8_t>& str) {
    write((uint32_t)str.size());
    memcpy(ptr_, str.data(), str.size());
    ptr_ += str.size();
  }

private:
  uint8_t* ptr_;
};

class packet {
public:
  const PacketType type;
  packet(PacketType type)
      : type(type) {}

  virtual size_t size() const = 0;
  virtual void serialize(uint8_t* ptr) const = 0;
};

template<PacketType T>
class client_packet : public packet {
public:
  client_packet() : packet(T) {}
};

template<PacketType T>
class server_packet : public packet {
public:
  server_packet() : packet(T) {}
};

class server_info_packet : public server_packet<PT_SERVER_INFO> {
public:
  uint32_t version;

  server_info_packet(uint32_t version)
      : version(version) {
  }

  server_info_packet(buffer_reader& reader) {
    reader.read(version);
  }

  size_t size() const override {
    return 1 + sizeof(version);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(version);
  }
};

class server_game_list_packet : public server_packet<PT_GAME_LIST> {
public:
  std::vector<std::pair<std::string, uint32_t>> games;

  server_game_list_packet() {}

  server_game_list_packet(buffer_reader& reader) {
    size_t count = reader.read<uint8_t>();
    while (count--) {
      uint32_t game_type = reader.read<uint32_t>();
      games.emplace_back(reader.read<std::string>(), game_type);
    }
  }

  size_t size() const override {
    size_t result = 2;
    for (const auto& game : games) {
      result += 1 + game.first.size() + sizeof(game.second);
    }
    return result;
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write((uint8_t) games.size());
    for (const auto& game : games) {
      writer.write(game.second);
      writer.write(game.first);
    }
  }
};

class server_join_accept_packet : public server_packet<PT_JOIN_ACCEPT> {
public:
  uint32_t cookie;
  uint8_t index;
  uint64_t init_info;

  server_join_accept_packet(uint32_t cookie, uint8_t index, uint64_t init_info)
      : cookie(cookie)
      , index(index)
      , init_info(init_info) {
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
    writer.write(type);
    writer.write(cookie);
    writer.write(index);
    writer.write(init_info);
  }
};

class server_join_reject_packet : public server_packet<PT_JOIN_REJECT> {
public:
  uint32_t cookie;
  RejectionReason reason;

  server_join_reject_packet(uint32_t cookie, RejectionReason reason)
      : cookie(cookie)
      , reason(reason) {
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
    writer.write(type);
    writer.write(cookie);
    writer.write(reason);
  }
};

class server_connect_packet : public server_packet<PT_CONNECT> {
public:
  uint8_t id;

  server_connect_packet(uint8_t id)
      : id(id) {
  }

  server_connect_packet(buffer_reader& reader) {
    reader.read(id);
  }

  size_t size() const override {
    return 1 + sizeof(id);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(id);
  }
};

class server_disconnect_packet : public server_packet<PT_DISCONNECT> {
public:
  uint8_t id;
  LeaveReason reason;

  server_disconnect_packet(uint8_t id, LeaveReason reason)
      : id(id)
      , reason(reason) {
  }

  server_disconnect_packet(buffer_reader& reader) {
    reader.read(id);
    reader.read(reason);
  }

  size_t size() const override {
    return 1 + sizeof(id) + sizeof(reason);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(id);
    writer.write(reason);
  }
};

class server_message_packet : public server_packet<PT_MESSAGE> {
public:
  uint8_t id;
  std::vector<uint8_t> payload;

  server_message_packet(uint8_t id, std::vector<uint8_t> payload)
      : id(id)
      , payload(std::move(payload)) {
  }

  server_message_packet(buffer_reader& reader) {
    reader.read(id);
    reader.read(payload);
  }

  size_t size() const override {
    return 5 + sizeof(id) + payload.size();
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(id);
    writer.write(payload);
  }
};

class server_turn_packet : public server_packet<PT_TURN> {
public:
  uint8_t id;
  uint32_t turn;

  server_turn_packet(uint8_t id, uint32_t turn)
    : id(id)
    , turn(turn)
  {
  }

  server_turn_packet(buffer_reader& reader) {
    reader.read(id);
    reader.read(turn);
  }

  size_t size() const override {
    return 1 + sizeof(id) + sizeof(turn);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(id);
    writer.write(turn);
  }
};

class client_game_list_packet : public client_packet<PT_GAME_LIST> {
public:
  client_game_list_packet() {}
  client_game_list_packet(buffer_reader&) {}

  size_t size() const override {
    return 1;
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
  }
};

class client_create_game_packet : public client_packet<PT_CREATE_GAME> {
public:
  uint32_t cookie;
  std::string name;
  std::string password;
  uint32_t difficulty;

  client_create_game_packet(uint32_t cookie, std::string name, std::string password, uint32_t difficulty)
    : cookie(cookie)
    , name(std::move(name))
    , password(std::move(password))
    , difficulty(difficulty)
  {
  }

  client_create_game_packet(buffer_reader& reader) {
    reader.read(cookie);
    reader.read(name);
    reader.read(password);
    reader.read(difficulty);
  }

  size_t size() const override {
    return 3 + sizeof(cookie) + name.size() + password.size() + sizeof(difficulty);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(cookie);
    writer.write(name);
    writer.write(password);
    writer.write(difficulty);
  }
};

class client_join_game_packet : public client_packet<PT_JOIN_GAME> {
public:
  uint32_t cookie;
  std::string name;
  std::string password;

  client_join_game_packet(uint32_t cookie, std::string name, std::string password)
      : cookie(cookie)
      , name(std::move(name))
      , password(std::move(password)) {
  }

  client_join_game_packet(buffer_reader& reader) {
    reader.read(cookie);
    reader.read(name);
    reader.read(password);
  }

  size_t size() const override {
    return 3 + sizeof(cookie) + name.size() + password.size();
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(cookie);
    writer.write(name);
    writer.write(password);
  }
};

class client_leave_game_packet : public client_packet<PT_LEAVE_GAME> {
public:
  client_leave_game_packet() {}
  client_leave_game_packet(buffer_reader&) {}

  size_t size() const override {
    return 1;
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
  }
};

class client_message_packet : public client_packet<PT_MESSAGE> {
public:
  uint8_t id;
  std::vector<uint8_t> payload;

  client_message_packet(uint8_t id, std::vector<uint8_t> payload)
      : id(id)
      , payload(std::move(payload)) {
  }

  client_message_packet(buffer_reader& reader) {
    reader.read(id);
    reader.read(payload);
  }

  size_t size() const override {
    return 5 + sizeof(id) + payload.size();
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(id);
    writer.write(payload);
  }
};

class client_turn_packet : public client_packet<PT_TURN> {
public:
  uint32_t turn;

  client_turn_packet(uint32_t turn)
    : turn(turn)
  {
  }

  client_turn_packet(buffer_reader& reader) {
    reader.read(turn);
  }

  size_t size() const override {
    return 1 + sizeof(turn);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(turn);
  }
};

class client_drop_player_packet : public client_packet<PT_DROP_PLAYER> {
public:
  uint8_t id;
  LeaveReason reason;

  client_drop_player_packet(uint8_t id, LeaveReason reason)
      : id(id)
      , reason(reason) {
  }

  client_drop_player_packet(buffer_reader& reader) {
    reader.read(id);
    reader.read(reason);
  }

  size_t size() const override {
    return 1 + sizeof(id) + sizeof(reason);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(id);
    writer.write(reason);
  }
};

class client_info_packet : public client_packet<PT_CLIENT_INFO> {
public:
  uint32_t version;

  client_info_packet(uint32_t version)
      : version(version) {
  }

  client_info_packet(buffer_reader& reader) {
    reader.read(version);
  }

  size_t size() const override {
    return 1 + sizeof(version);
  }

  void serialize(uint8_t* ptr) const override {
    buffer_writer writer(ptr);
    writer.write(type);
    writer.write(version);
  }
};
