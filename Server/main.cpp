#include <boost/asio/signal_set.hpp>
#include <mutex>
#include <random>
#include <thread>

#include "server.hpp"
#include "packet.hpp"

const size_t MAX_PLAYERS = 4;

const uint8_t PLR_MASTER = 0xFE;
const uint8_t PLR_BROADCAST = 0xFF;

class player_client;
class game_instance;

const uint32_t SERVER_VERSION = 1;

uint64_t make_init_info(uint32_t difficulty) {
  static std::mt19937 mt((uint32_t) time(0));
  uint64_t result;
  uint32_t* data = reinterpret_cast<uint32_t*>(&result);
  data[0] = mt();
  data[1] = difficulty;
  return result;
}

class game_server : public websocket_server {
public:
  using websocket_server::websocket_server;

  void get_game_list(uint32_t version, std::vector<std::pair<std::string, uint32_t>>& list);
  RejectionReason create_game(uint32_t version, const std::string& name, const std::string& password, uint32_t type);
  void close_game(const std::string& name);
  std::shared_ptr<game_instance> find_game(const std::string& name);

  void on_connect(tcp::socket&& socket) override;

private:
  std::recursive_mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<game_instance>> games_;
};

class game_instance {
public:
  const uint32_t version;
  const std::string name;
  const std::string password;
  const uint32_t difficulty;
  const uint64_t init_info;

  game_instance(std::shared_ptr<game_server> server, uint32_t version, std::string name, std::string password, uint32_t difficulty)
      : server_(server)
      , version(version)
      , name(std::move(name))
      , password(std::move(password))
      , difficulty(difficulty)
      , init_info(make_init_info(difficulty))
  {
  }

  bool full();

  void send(uint32_t mask, const packet& p);

  void drop_player(uint8_t id, LeaveReason reason);

  uint8_t join(std::shared_ptr<player_client> player, uint32_t cookie);

private:
  std::recursive_mutex mutex_;
  std::shared_ptr<game_server> server_;
  std::array<std::shared_ptr<player_client>, MAX_PLAYERS> players_;
};

class player_client : public websocket_client {
public:
  player_client(std::shared_ptr<game_server> server, tcp::socket&& socket)
      : websocket_client(std::move(socket))
      , server_(server) {
  }

  ~player_client();

  void on_connect() override {
    send(server_info_packet(SERVER_VERSION));
  }

  void on_receive(const uint8_t* data, size_t size);

  void join(uint32_t cookie, const std::string& name, const std::string& password);

  void on_leave() {
    game_ = nullptr;
    id_ = PLR_BROADCAST;
  }

private:
  std::recursive_mutex mutex_;
  std::shared_ptr<game_server> server_;
  std::shared_ptr<game_instance> game_;
  uint8_t id_ = PLR_BROADCAST;
  uint32_t version_ = 0;
};

void game_server::get_game_list(uint32_t version, std::vector<std::pair<std::string, uint32_t>>& list) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  for (const auto& game : games_) {
    if (game.second->version == version && !game.second->full()) {
      list.emplace_back(game.first, game.second->difficulty | (game.second->password.size() ? 0x80000000 : 0));
    }
  }
}

RejectionReason game_server::create_game(uint32_t version, const std::string& name, const std::string& password, uint32_t difficulty) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = games_.find(name);
  if (it != games_.end()) {
    return CREATE_GAME_EXISTS;
  }
  games_.emplace_hint(it, name, std::make_shared<game_instance>(std::static_pointer_cast<game_server>(shared_from_this()), version, name, password, difficulty));
  return JOIN_SUCCESS;
}

void game_server::close_game(const std::string& name) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  games_.erase(name);
}

std::shared_ptr<game_instance> game_server::find_game(const std::string& name) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto it = games_.find(name);
  return it != games_.end() ? it->second : nullptr;
}

void game_server::on_connect(tcp::socket&& socket) {
  std::make_shared<player_client>(std::static_pointer_cast<game_server>(shared_from_this()), std::move(socket))->listen();
}

bool game_instance::full() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  for (const auto& p : players_) {
    if (!p) {
      return false;
    }
  }
  return true;
}

uint8_t game_instance::join(std::shared_ptr<player_client> player, uint32_t cookie) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  for (size_t index = 0; index < players_.size(); ++index) {
    if (!players_[index]) {
      players_[index] = player;
      player->send(server_join_accept_packet(cookie, (uint8_t) index, init_info));
      send(PLR_BROADCAST, server_connect_packet((uint8_t) index));
      return (uint8_t) index;
    }
  }
  player->send(server_join_reject_packet(cookie, JOIN_GAME_FULL));
  return PLR_BROADCAST;
}

void game_instance::drop_player(uint8_t id, LeaveReason reason) {
  std::shared_ptr<game_server> server;
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    send(PLR_BROADCAST, server_disconnect_packet(id, reason));
    if (players_[id]) {
      players_[id]->on_leave();
    }
    players_[id] = nullptr;
    for (size_t i = 0; i < players_.size(); ++i) {
      if (players_[i]) {
        return;
      }
    }
    server = server_;
  }
  server->close_game(name);
}

void game_instance::send(uint32_t mask, const packet& p) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto buffer = make_buffer(p);
  for (size_t index = 0; index < players_.size(); ++index) {
    if (players_[index] && players_[index]->is_open() && (mask & (1u << index))) {
      players_[index]->send(buffer);
    }
  }
}

void player_client::on_receive(const uint8_t* data, size_t size) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  try {
    buffer_reader reader(data, size);
    switch (reader.read<PacketType>()) {
    case PT_CLIENT_INFO: {
      client_info_packet packet(reader);
      version_ = packet.version;
      break;
    }
    case PT_GAME_LIST: {
      client_game_list_packet packet(reader);
      if (!game_) {
        server_game_list_packet response;
        server_->get_game_list(version_, response.games);
        send(response);
      }
      break;
    }
    case PT_CREATE_GAME: {
      client_create_game_packet packet(reader);
      if (game_) {
        send(server_join_reject_packet(packet.cookie, JOIN_ALREADY_IN_GAME));
      } else {
        if (auto reason = server_->create_game(version_, packet.name, packet.password, packet.difficulty)) {
          send(server_join_reject_packet(packet.cookie, reason));
        } else {
          join(packet.cookie, packet.name, packet.password);
        }
      }
      break;
    }
    case PT_JOIN_GAME: {
      client_join_game_packet packet(reader);
      if (game_) {
        send(server_join_reject_packet(packet.cookie, JOIN_ALREADY_IN_GAME));
      } else {
        join(packet.cookie, packet.name, packet.password);
      }
      break;
    }
    case PT_LEAVE_GAME: {
      client_leave_game_packet packet(reader);
      if (game_) {
        game_->drop_player(id_, LEAVE_NORMAL);
      }
      break;
    }
    case PT_DROP_PLAYER: {
      client_drop_player_packet packet(reader);
      if (game_) {
        game_->drop_player(packet.id, packet.reason);
      }
      break;
    }
    case PT_MESSAGE: {
      client_message_packet packet(reader);
      if (game_) {
        game_->send(packet.id == PLR_BROADCAST ? ~(1u << id_) : (1u << packet.id), server_message_packet(id_, packet.payload));
      }
      break;
    }
    case PT_TURN: {
      client_turn_packet packet(reader);
      if (game_) {
        game_->send(~(1u << id_), server_turn_packet(id_, packet.turn));
      }
      break;
    }
    default:
      close(websocket::close_code::bad_payload);
    }
    if (!reader.done()) {
      close(websocket::close_code::bad_payload);
    }
  } catch (parse_error pe) {
    close(websocket::close_code::bad_payload);
  }
}

player_client::~player_client() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (game_) {
    game_->drop_player(id_, LEAVE_DROP);
  }
}

void player_client::join(uint32_t cookie, const std::string& name, const std::string& password) {
  auto game = server_->find_game(name);
  if (!game) {
    send(server_join_reject_packet(cookie, JOIN_GAME_NOT_FOUND));
  } else if (game->version != version_) {
    send(server_join_reject_packet(cookie, JOIN_VERSION_MISMATCH));
  } else if (game->password != password) {
    send(server_join_reject_packet(cookie, JOIN_INCORRECT_PASSWORD));
  } else {
    auto id = game->join(std::static_pointer_cast<player_client>(shared_from_this()), cookie);
    if (id != PLR_BROADCAST) {
      game_ = game;
      id_ = id;
    }
  }
}

int main(int argc, char* argv[]) {
  const int threads = 4;
  boost::asio::io_context ioc(threads);

  auto server = std::make_shared<game_server>(ioc, 1339);
  server->listen();

  net::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait([&](error_code, int) {
    ioc.stop();
  });

  std::vector<std::thread> v;
  v.reserve(threads - 1);
  for (auto i = threads - 1; i > 0; --i) {
    v.emplace_back([&ioc] {
      ioc.run();
    });
  }
  ioc.run();

  for (auto& t : v) {
    t.join();
  }

  return EXIT_SUCCESS;
}
