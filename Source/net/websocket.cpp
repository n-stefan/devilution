#ifndef EMSCRIPTEN
#include "websocket_native.hpp"
#endif

#include "websocket.h"
#include "../ui/network.h"

const uint32_t SERVER_VERSION = 1;

namespace net {

websocket_client::websocket_client()
  : impl_(std::make_unique<websocket_impl>())
{
  impl_->send(client_info_packet(gdwProductVersion));
}

void websocket_client::create(std::string name, std::string passwd, uint32_t difficulty) {
  cookie_ = (uint32_t) time(nullptr);
  impl_->send(client_create_game_packet(cookie_, name, passwd, (uint8_t) difficulty));
}

void websocket_client::join(std::string name, std::string passwd) {
  cookie_ = (uint32_t) time(nullptr);
  impl_->send(client_join_game_packet(cookie_, name, passwd));
}

void websocket_client::poll() {
  impl_->receive([this](const uint8_t* data, size_t size) {
    handle_packet(data, size);
  });
  if (!closed_ && impl_->is_closed()) {
    closed_ = true;
    NetworkState* net_state = dynamic_cast<NetworkState*>(GameState::current());
    if (net_state) {
      net_state->onClosed();
    }
  }
}

void websocket_client::send(const packet& pkt) {
  impl_->send(pkt);
}

void websocket_client::handle_packet(const uint8_t* data, size_t size) {
  try {
    buffer_reader reader(data, size);
    NetworkState* net_state = dynamic_cast<NetworkState*>(GameState::current());
    PacketType type = reader.read<PacketType>();
    switch (type) {
    case PT_SERVER_INFO:
    {
      server_info_packet packet(reader);
      if (packet.version != SERVER_VERSION) {
        app_fatal("Server version mismatch. Expected %u, received %u.", SERVER_VERSION, packet.version);
      }
      break;
    }
    case PT_GAME_LIST:
    {
      server_game_list_packet packet(reader);
      break;
    }
    case PT_JOIN_ACCEPT:
    {
      server_join_accept_packet packet(reader);
      if (packet.cookie == cookie_ && net_state) {
        // handle first, so we read the init info
        handle(packet);
        net_state->onJoinAccept(packet.index);
      } else {
        // tell the server we don't want this game anymore
        send(client_leave_game_packet());
      }
      break;
    }
    case PT_JOIN_REJECT:
    {
      server_join_reject_packet packet(reader);
      if (packet.cookie == cookie_ && net_state) {
        net_state->onJoinReject(packet.reason);
      }
      break;
    }
    case PT_CONNECT:
      handle(server_connect_packet(reader));
      break;
    case PT_DISCONNECT:
      handle(server_disconnect_packet(reader));
      break;
    case PT_MESSAGE:
      handle(server_message_packet(reader));
      break;
    case PT_TURN:
      handle(server_turn_packet(reader));
      break;
    default:
      app_fatal("Unrecognized server message type %d", (int) type);
    }
    if (!reader.done()) {
      app_fatal("Invalid server message size");
    }
  } catch (parse_error) {
    app_fatal("Error parsing server message");
  }
}

}
