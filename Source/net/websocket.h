#pragma once

#include <string>
#include <memory>

#include "base.h"

class websocket_impl;

namespace net {

class websocket_client : public base {
public:
  websocket_client();
  ~websocket_client();

  void create(std::string name, std::string passwd, uint32_t difficulty) override;
  void join(std::string name, std::string passwd) override;

  void poll() override;
  void send(const packet& pkt) override;

private:
  uint32_t cookie_;
  bool closed_ = false;
  std::unique_ptr<websocket_impl> impl_;
  void handle_packet(const uint8_t* data, size_t size);
};

}  // namespace net
