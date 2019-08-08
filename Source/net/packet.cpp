#include "packet.h"

namespace net {

const buffer_t& packet::data()
{
  assert(have_decrypted && have_encrypted);
	return encrypted_buffer;
}

packet_type packet::type()
{
  assert(have_decrypted);
	return m_type;
}

plr_t packet::src()
{
  assert(have_decrypted);
	return m_src;
}

plr_t packet::dest()
{
  assert(have_decrypted);
  return m_dest;
}

const buffer_t& packet::message()
{
  assert(have_decrypted);
  if (m_type != PT_MESSAGE)
    ERROR_MSG("incorrect message type");
  return m_message;
}

turn_t packet::turn()
{
  assert(have_decrypted);
  if (m_type != PT_TURN)
    ERROR_MSG("incorrect message type");
  return m_turn;
}

cookie_t packet::cookie()
{
  assert(have_decrypted);
  if (m_type != PT_JOIN_REQUEST && m_type != PT_JOIN_ACCEPT)
    ERROR_MSG("incorrect message type");
  return m_cookie;
}

plr_t packet::newplr()
{
  assert(have_decrypted);
  if (m_type != PT_JOIN_ACCEPT && m_type != PT_CONNECT
	    && m_type != PT_DISCONNECT)
    ERROR_MSG("incorrect message type");
  return m_newplr;
}

const buffer_t& packet::info()
{
  assert(have_decrypted);
  if (m_type != PT_JOIN_REQUEST && m_type != PT_JOIN_ACCEPT)
    ERROR_MSG("incorrect message type");
  return m_info;
}

leaveinfo_t packet::leaveinfo()
{
  assert(have_decrypted);
  if (m_type != PT_DISCONNECT)
    ERROR_MSG("incorrect message type");
  return m_leaveinfo;
}

void packet_in::create(buffer_t buf)
{
  assert(!have_decrypted && !have_encrypted);
	encrypted_buffer = std::move(buf);
	have_encrypted = true;
}

void packet_in::decrypt()
{
  assert(have_encrypted);
	if (have_decrypted)
		return;
	if (encrypted_buffer.size() < sizeof(packet_type) + 2*sizeof(plr_t))
    ERROR_MSG("invalid buffer size");
  decrypted_buffer = encrypted_buffer;

	process_data();

	have_decrypted = true;
}

void packet_out::encrypt()
{
  assert(have_decrypted);
  if (have_encrypted)
		return;

	process_data();

  encrypted_buffer = decrypted_buffer;
	have_encrypted = true;
}

packet_factory::packet_factory(std::string pw)
{
}

}  // namespace net
