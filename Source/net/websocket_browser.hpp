#include <emscripten.h>
#include <vector>
#include <functional>
#include "../../Server/packet.hpp"
#include "../trace.h"

EM_JS(void, api_websocket_send, (const void* ptr, int size), {
  self.DApi.websocket_send(HEAPU8.subarray(ptr, ptr + size));
});
EM_JS(int, api_websocket_closed, (), {
  return self.DApi.websocket_closed();
});

class websocket_impl;

static websocket_impl* impl_ = nullptr;

class websocket_impl {
public:
  websocket_impl() {
    impl_ = this;
  }
  ~websocket_impl() {
    impl_ = nullptr;
  }

  void send(const packet& p) {
    size_t size = p.size();
    pkt_.resize(size);
    p.serialize(pkt_.data());
    api_websocket_send(pkt_.data(), pkt_.size());
  }

  bool is_closed() {
    return api_websocket_closed();;
  }

  void receive(std::function<void(const uint8_t*, size_t)> handler) {
    for (auto& pkt : pending_) {
      handler(pkt.data(), pkt.size());
    }
    pending_.clear();
  }

  void* alloc_packet(int size) {
    pending_.emplace_back((size_t) size);
    return pending_.back().data();
  }

private:
  std::vector<uint8_t> pkt_;
  std::vector<std::vector<uint8_t>> pending_;
};

extern "C" {

  EMSCRIPTEN_KEEPALIVE
    void* DApi_AllocPacket(int size) {
    if (impl_) {
      return impl_->alloc_packet(size);
    } else {
      return nullptr;
    }
  }

}
