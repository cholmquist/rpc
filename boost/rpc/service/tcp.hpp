#ifndef BOOST_RPC_SERVICE_TCP_HPP
#define BOOST_RPC_SERVICE_TCP_HPP

#include <boost/rpc/service/stream_header.hpp>
#include <boost/rpc/detail/switch_.hpp>
#include <boost/rpc/detail/header.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/array.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace boost{ namespace rpc{ namespace service {

	namespace detail
	{
		struct const_header_data : boost::static_visitor<asio::const_buffer>
		{
			template<class Header>
			asio::const_buffer operator()(const Header& h) const
			{
				return asio::const_buffer(h.data.data(), h.data.size());
			}
		};

	}

template<class Header, class Allocator = std::allocator<char> >
class tcp
{
public:
	typedef Header header_type;
	typedef system::error_code error_type;
	typedef asio::ip::tcp::socket socket_type;

	tcp(asio::io_service& ios)
		: m_socket(ios)
	{

	}

	error_type send(const header_type& header, const asio::const_buffer& payload)
	{
		system::error_code ec;
		array<asio::const_buffer, 2> buffers;
		buffers[0] = boost::apply_visitor(detail::const_header_data(), header);
		buffers[1] = payload;
		m_socket.send(buffers, 0, ec);
		return ec;
	}

	error_type receive(header_type& h, asio::const_buffer& buffer)
	{
		system::error_code ec;
		m_recv_buffer.reset2();
		std::size_t length = m_socket.read_some(
			asio::mutable_buffers_1(m_recv_buffer.prepare()));
/*		for(std::size_t i = 0; i < length; ++i)
		{
			switch(m_decoder.process(m_recv_buffer[i]))
			{
			case rpc::detail::ds_init:
				break;
			}
		}*/
		return error_type();
	}

	socket_type& socket() { return m_socket; }

private:

	typedef rpc::detail::header<Header> header_details;

	asio::ip::tcp::socket	m_socket;
	typename header_details::storage_type	m_recv_buffer;
};


}}}

#endif
