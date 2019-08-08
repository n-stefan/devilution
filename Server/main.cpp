//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket server, asynchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <deque>
#include <string_view>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;
using error_code = boost::system::error_code;

void fail(error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}
#define check_ec(ec, what) if (ec) {fail(ec, what); return;}

class websocket_client : public std::enable_shared_from_this<websocket_client> {
public:
  explicit websocket_client(tcp::socket&& socket, std::chrono::milliseconds timeout = std::chrono::seconds(15))
    : ws_(std::move(socket))
    , strand_(ws_.get_executor())
    , timer_(ws_.get_executor().context(), std::chrono::steady_clock::time_point::max())
    , timeout_(timeout)
  {
  }

  bool is_open() const {
    return ws_.is_open();
  }

  void listen() {
    ws_.control_callback([this](websocket::frame_type type, beast::string_view) {
      if (type == websocket::frame_type::close) {
        on_close();
      } else {
        activity();
      }
    });
    ws_.async_accept(net::bind_executor(strand_, [self = shared_from_this()](error_code ec) {
      check_ec(ec, "accept");
      self->accepted_ = true;
      if (!self->output_.empty()) {
        self->do_write();
      }
      self->on_connect();
      self->do_read();
    }));
  }

  void send(const void* data, size_t size) {
    beast::flat_buffer output;
    auto dest = output.prepare(size);
    memcpy(dest.data(), data, size);
    output.commit(size);

    if (strand_.running_in_this_thread() && accepted_) {
      output_.emplace_back(std::move(output));
      if (output_.size() == 1) {
        do_write();
      }
    } else {
      net::post(net::bind_executor(strand_, [self = shared_from_this(), output = std::move(output)]()
      {
        self->output_.emplace_back(std::move(output));
        if (self->output_.size() == 1) {
          self->do_write();
        }
      }));
    }
  }

protected:
  virtual void on_connect() {}
  virtual void on_close() {}
  virtual void on_receive(const uint8_t* data, size_t size) = 0;

private:
  void do_read() {
    ws_.async_read(input_, net::bind_executor(strand_, [self = shared_from_this()](error_code ec, size_t) {
      if (ec == websocket::error::closed || ec == net::error::connection_aborted) {
        return;
      }
      check_ec(ec, "read");

      self->activity();

      auto data = self->input_.data();
      self->on_receive((uint8_t*) data.data(), data.size());

      self->input_.consume(data.size());
      self->do_read();
    }));
  }

  void do_write() {
    ws_.text(true);
    ws_.async_write(output_.front().data(), net::bind_executor(strand_, [self = shared_from_this()](error_code ec, size_t) {
      if (ec == websocket::error::closed || ec == net::error::connection_aborted) {
        return;
      }
      check_ec(ec, "write");
      self->output_.pop_front();
      if (!self->output_.empty()) {
        self->do_write();
      }
    }));
  }

  void on_timer(error_code ec) {
    if (ec != net::error::operation_aborted) {
      check_ec(ec, "timer");
    }
    if (timer_.expiry() <= std::chrono::steady_clock::now()) {
      if (ws_.is_open() && ping_state_ == 0) {
        ping_state_ = 1;
        timer_.expires_after(timeout_);
        ws_.async_ping({}, net::bind_executor(strand_, [self = shared_from_this()](error_code ec) {
          if (ec == net::error::operation_aborted) {
            return;
          }
          check_ec(ec, "ping");
          if (self->ping_state_ == 1) {
            self->ping_state_ = 2;
          } else {
            BOOST_ASSERT(self->ping_state_ == 0);
          }
        }));
      } else {
        ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
        ws_.next_layer().close();
        return;
      }
    }
    timer_.async_wait(net::bind_executor(strand_, std::bind(&websocket_client::on_timer, shared_from_this(), std::placeholders::_1)));
  }

  void activity() {
    ping_state_ = 0;
    timer_.expires_after(timeout_);
  }

  websocket::stream<tcp::socket> ws_;
  net::strand<net::io_context::executor_type> strand_;
  net::steady_timer timer_;
  std::chrono::milliseconds timeout_;
  int ping_state_ = 0;
  beast::flat_buffer input_;
  std::deque<beast::flat_buffer> output_;
  std::atomic_bool accepted_ = false;
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

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  std::unordered_map<size_t, std::shared_ptr<broadcast_client>> clients_;

public:
  listener(
    boost::asio::io_context& ioc,
    tcp::endpoint endpoint)
    : acceptor_(ioc)
    , socket_(ioc) {
    boost::system::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      fail(ec, "open");
      return;
    }

    // Allow address reuse
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      fail(ec, "set_option");
      return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
      fail(ec, "bind");
      return;
    }

    // Start listening for connections
    acceptor_.listen(
      boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
      fail(ec, "listen");
      return;
    }
  }

  // Start accepting incoming connections
  void
    run() {
    if (!acceptor_.is_open())
      return;
    do_accept();
  }

  void do_accept() {
    acceptor_.async_accept(
      socket_,
      std::bind(
        &listener::on_accept,
        shared_from_this(),
        std::placeholders::_1));
  }

  void broadcast(std::string_view msg, size_t id) {
    std::cout << "sending in tid " << std::this_thread::get_id() << std::endl;
    for (auto& kv : clients_) {
      if (id != kv.first) {
        kv.second->send(msg.data(), msg.size());
      }
    }
  }

  void on_accept(boost::system::error_code ec) {
    if (ec) {
      fail(ec, "accept");
    } else {
      // Create the session and run it
      std::cout << "accepting in tid " << std::this_thread::get_id() << std::endl;
      size_t cid = clients_.size();
      auto client = std::make_shared<broadcast_client>(std::move(socket_), std::bind(&listener::broadcast, shared_from_this(), std::placeholders::_1, cid));
      clients_.emplace(cid, client);
      client->listen();
    }

    // Accept another connection
    do_accept();
  }
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  const auto address = boost::asio::ip::make_address("0.0.0.0");
  const unsigned short port = 1339;
  const int threads = 4;

  // The io_context is required for all I/O
  boost::asio::io_context ioc{threads};

  // Create and launch a listening port
  std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> v;
  v.reserve(threads - 1);
  for (auto i = threads - 1; i > 0; --i)
    v.emplace_back(
      [&ioc] {
    ioc.run();
  });
  ioc.run();

  return EXIT_SUCCESS;
}
