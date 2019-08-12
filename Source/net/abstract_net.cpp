#include "abstract_net.h"
#include "loopback.h"
#include "websocket.h"

namespace net {

abstract_net::~abstract_net()
{
}

std::unique_ptr<abstract_net> abstract_net::make_net(provider_t provider, const char* param) {
  if (provider == provider_t::LOOPBACK) {
    return std::make_unique<loopback>();
  } else if (provider == provider_t::WEBSOCKET) {
    return std::make_unique<websocket_client>();
	} else {
		ERROR_MSG("unknown provider type");
    return std::make_unique<loopback>();
	}
}

}  // namespace net
