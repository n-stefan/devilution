#include <boost/asio/signal_set.hpp>
#include <mutex>
#include <string_view>

#include "server.hpp"

const size_t MAX_PLAYERS = 4;

typedef uint8_t plr_t;

const plr_t PLR_MASTER = 0xFE;
const plr_t PLR_BROADCAST = 0xFF;

class game_server;

class player_client : public websocket_client {
public:
  std::shared_ptr<game_server> game_;
  plr_t id_;
};

class game_server {
public:
  void on_message(plr_t id, const uint8_t* data, size_t size);

private:
  std::array<std::shared_ptr<player_client>, MAX_PLAYERS> players_;
};

class broadcast_client : public websocket_client {
public:
  explicit broadcast_client(tcp::socket&& socket, std::function<void(std::string_view)> callback)
    : websocket_client(std::move(socket))
    , callback_(callback)
  {
  }

  void on_close() override {
    std::cout << "closed" << std::endl;
  }

  void on_receive(const uint8_t* data, size_t size) override {
    callback_(std::string_view((char*) data, size));
  }

private:
  std::function<void(std::string_view)> callback_;
};

class broadcast_server : public websocket_server {
public:
  broadcast_server(boost::asio::io_context& ioc, unsigned short port)
    : websocket_server(ioc, port)
  {
  }

  void on_connect(tcp::socket&& socket) override {
    std::unique_lock lock(mutex_);
    size_t id = id_++;
    auto client = std::make_shared<broadcast_client>(std::move(socket), [self = shared_from_this(), id](std::string_view msg) {
      std::static_pointer_cast<broadcast_server>(self)->broadcast(msg, id);
    });
    clients_.emplace(id, client);
    client->listen();
  }

  void broadcast(std::string_view msg, size_t id) {
    std::unique_lock lock(mutex_);
    for (auto& kv : clients_) {
      if (id != kv.first) {
        kv.second->send(msg.data(), msg.size());
      }
    }
  }

private:
  std::mutex mutex_;
  size_t id_ = 0;
  std::unordered_map<size_t, std::shared_ptr<broadcast_client>> clients_;
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  const int threads = 4;
  boost::asio::io_context ioc(threads);

  auto server = std::make_shared<broadcast_server>(ioc, 1339);
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
