#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <deque>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;
using error_code = boost::system::error_code;

void fail(error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}
#define check_ec(ec, what)                                                     \
  if (ec) {                                                                    \
    fail(ec, what);                                                            \
    return;                                                                    \
  }

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

  template<class packet>
  void send(const packet& packet) {
    beast::flat_buffer output;
    size_t size = packet.size();
    auto dest = output.prepare(size);
    packet.serialize((uint8_t*) dest.data());
    output.commit(size);

    if (strand_.running_in_this_thread() && accepted_) {
      output_.emplace_back(std::move(output));
      if (output_.size() == 1) {
        do_write();
      }
    } else {
      net::post(net::bind_executor(strand_, [self = shared_from_this(), output = std::move(output)]() {
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
    ws_.text(false);
    ws_.async_write(output_.front().data(),net::bind_executor(strand_,[self = shared_from_this()](error_code ec, size_t) {
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

class websocket_server : public std::enable_shared_from_this<websocket_server> {
public:
  websocket_server(boost::asio::io_context& ioc, unsigned short port)
    : acceptor_(ioc)
    , socket_(ioc)
  {
    error_code ec;
    tcp::endpoint endpoint(net::ip::address_v4(0u), port);

    acceptor_.open(endpoint.protocol(), ec);
    check_ec(ec, "open");

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    check_ec(ec, "open");

    acceptor_.bind(endpoint, ec);
    check_ec(ec, "bind");

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    check_ec(ec, "listen");
  }

  void listen() {
    if (!acceptor_.is_open()) {
      return;
    }
    do_accept();
  }

protected:
  virtual void on_connect(tcp::socket&& socket) = 0;

private:
  void do_accept() {
    acceptor_.async_accept(socket_, [self = shared_from_this()](error_code ec) {
      if (ec) {
        fail(ec, "accept");
      } else {
        self->on_connect(std::move(self->socket_));
      }
      self->do_accept();
    });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
};
