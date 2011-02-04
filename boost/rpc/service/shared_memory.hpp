#ifndef BOOST_RPC_SERVICE_SHARED_MEMORY_HPP
#define BOOST_RPC_SERVICE_SHARED_MEMORY_HPP

#include <boost/interprocess/ipc/message_queue.hpp>

namespace boost{ namespace rpc{ namespace service {

template<class Header>
class shared_memory
{
public:

	typedef Header header_type;
	typedef std::vector<char> payload_type;

	shared_memory(const std::string& name)
		: m_queue(interprocess::open_or_create, name.c_str(), 16, 1024)
	{

	}

	void send(const header_type& h, const payload_type& p)
	{
		std::vector<char> hbuf;
		hbuf.resize(sizeof(h));
		std::memcpy(&hbuf[0], &h, sizeof()h);
	}

private:
	interprocess::message_queue m_queue;
};

}}}

#endif
