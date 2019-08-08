#include "abstract_net.h"
#include "loopback.h"

namespace net {

abstract_net::~abstract_net()
{
}

std::unique_ptr<abstract_net> abstract_net::make_net(provider_t provider) {
	/*if (provider == 'TCPN') {
		return std::make_unique<tcp_client>();
	} else if (provider == 'UDPN') {
		return std::make_unique<udp_p2p>();
	} else */if (provider == provider_t::LOOPBACK) {
		return std::make_unique<loopback>();
	} else {
		ERROR_MSG("unknown provider type");
	}
}

}  // namespace net
