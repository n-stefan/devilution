#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _WIN32_WINNT 0x0501

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <future>

#include "../appfat.h"
#include "../../Server/packet.hpp"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;
using error_code = boost::system::error_code;

beast::flat_buffer make_buffer(const packet& p) {
  beast::flat_buffer result;
  size_t size = p.size();
  auto dest = result.prepare(size);
  p.serialize((uint8_t*) dest.data());
  result.commit(size);
  return result;
}

void fail(error_code ec, char const* what) {
  app_fatal("%s: %s", what, ec.message().c_str());
}
#define check_ec(ec, what) \
  if (ec) {                \
    fail(ec, what);        \
    return;                \
  }

class websocket_session : public std::enable_shared_from_this<websocket_session> {
public:
  explicit websocket_session(boost::asio::io_context& ioc)
    : resolver_(ioc)
    , ws_(ioc)
    , strand_(ws_.get_executor())
  {
  }

  bool is_closed() {
    return closed_;
  }

  void connect(std::promise<std::shared_ptr<websocket_session>>&& result, std::string host, unsigned short port, std::string target = "/") {
    tcp::endpoint endpoint(boost::asio::ip::make_address(host), port);
    ws_.control_callback([this](websocket::frame_type type, beast::string_view) {
      if (type == websocket::frame_type::close) {
        closed_ = true;
      }
    });
    resolver_.async_resolve(endpoint, [self = shared_from_this(), host, target, result = std::move(result)](error_code ec, tcp::resolver::results_type results) mutable {
      check_ec(ec, "resolve");
      boost::asio::async_connect(self->ws_.next_layer(), results.begin(), results.end(), [self, host, target, result = std::move(result)](error_code ec, const auto&) mutable {
        check_ec(ec, "connect");
        self->ws_.async_handshake(host, target, [self, result = std::move(result)](error_code ec) mutable {
          check_ec(ec, "handshake");

          std::lock_guard lock(self->mutex_);
          self->ready_ = true;
          if (!self->output_.empty()) {
            self->do_write();
          }
          self->do_read();
          result.set_value(self);
        });
      });
    });
  }

  void send(beast::flat_buffer buffer) {
    boost::asio::post(boost::asio::bind_executor(strand_, [self = shared_from_this(), buffer = std::move(buffer)]() {
      self->output_.emplace_back(std::move(buffer));
      if (self->output_.size() == 1 && self->ready_) {
        self->do_write();
      }
    }));
  }

  void send(const packet& packet) {
    send(make_buffer(packet));
  }

  void receive(std::function<void(const uint8_t*, size_t)> handler) {
    std::lock_guard lock(mutex_);
    for (auto& buffer : input_) {
      handler(buffer.data(), buffer.size());
    }
    input_.clear();
  }

private:
  std::mutex mutex_;
  websocket::stream<tcp::socket> ws_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  std::atomic_bool ready_ = false;
  std::atomic_bool closed_ = false;
  tcp::resolver resolver_;
  std::vector<std::vector<uint8_t>> input_;
  beast::flat_buffer next_input_;
  std::deque<beast::flat_buffer> output_;

  void do_write() {
    ws_.text(false);
    ws_.async_write(output_.front().data(), [self = shared_from_this()](error_code ec, size_t) {
      if (ec == websocket::error::closed || ec == boost::asio::error::connection_aborted) {
        return;
      }
      check_ec(ec, "write");
      self->output_.pop_front();
      if (!self->output_.empty()) {
        self->do_write();
      }
    });
  }

  void do_read() {
    ws_.async_read(next_input_, [self = shared_from_this()](error_code ec, size_t) {
      if (ec == websocket::error::closed || ec == boost::asio::error::connection_aborted) {
        return;
      }
      check_ec(ec, "read");

      auto data = self->next_input_.data();
      std::vector<uint8_t> input((uint8_t*) data.data(), (uint8_t*) data.data() + data.size());
      self->next_input_.consume(data.size());

      {
        std::lock_guard lock(self->mutex_);
        self->input_.emplace_back(std::move(input));
      }

      self->do_read();
    });
  }
};

class websocket_impl {
public:
  websocket_impl() {
    std::promise<std::shared_ptr<websocket_session>> promise;
    auto future = promise.get_future();
    thread_ = std::thread([promise = std::move(promise)]() mutable {
      boost::asio::io_context ioc(1);
      std::make_shared<websocket_session>(ioc)->connect(std::move(promise), "127.0.0.1", 3001, "/");
      ioc.run();
    });
    session_ = future.get();
  }

  void send(const packet& p) {
    session_->send(p);
  }

  bool is_closed() {
    return session_->is_closed();
  }

  void receive(std::function<void(const uint8_t*, size_t)> handler) {
    session_->receive(std::move(handler));
  }

private:
  std::thread thread_;
  std::shared_ptr<websocket_session> session_;
};
