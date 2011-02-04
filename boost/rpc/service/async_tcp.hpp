#ifndef BOOST_RPC_SERVICE_ASYNC_STREAM_HPP
#define BOOST_RPC_SERVICE_ASYNC_STREAM_HPP

#include <boost/rpc/service/stream_header.hpp>
#include <boost/rpc/core/tags.hpp> // TODO: remove
#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/function/function3.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/checked_delete.hpp>
#include <boost/intrusive/slist.hpp>

namespace boost{ namespace rpc{
	
	namespace detail
	{
		template<class Handler>
		struct packet : intrusive::make_slist_base_hook<>::type
		{
			typedef std::vector<char> buffer_type;

			typedef typename boost::intrusive::make_slist<packet,
				intrusive::cache_last<true>, intrusive::constant_time_size<false> >::type list_type;

			template<class Header, class Serialize>
			packet(const Header& header, buffer_type& payload, const Handler& handler, Serialize& serialize)
				: m_handler(handler)
			{
				Serialize::writer(serialize, m_header)(header, rpc::tags::default_());
				m_payload.swap(payload);
				m_control_buffer = control_data::encode(m_control, m_header.size(), m_payload.size());
			}

			boost::array<asio::const_buffer, 3> to_buffers() const
			{
				boost::array<asio::const_buffer, 3> buffers;
				buffers[0] = m_control_buffer;
				if(!m_header.empty())
				{
					buffers[1] = asio::const_buffer(&m_header[0], m_header.size());
				}
				if(!m_payload.empty())
				{
					buffers[2] = asio::const_buffer(&m_payload[0], m_payload.size());
				}
				return buffers;
			}

			control_data::buffer_type m_control;
			asio::const_buffer m_control_buffer;
			buffer_type m_header;
			buffer_type m_payload;
			Handler m_handler;

		private:
			packet(const packet&);
			packet& operator=(const packet&);
		};

	}

namespace service {

enum receive_state
{
	rs_init,
	rs_control_data,
	rs_header_buffer,
	rs_payload_buffer,
};

template<class Derived, class Header, class Stream, class Serialize>
class async_asio_stream
{
public:

	typedef async_asio_stream<Derived, Header, Stream, Serialize> async_stream_base;
	typedef std::vector<char> buffer_type;
	typedef boost::function<void(boost::shared_ptr<Derived>, buffer_type&, system::error_code)> async_handler;
	typedef Header header_type;
	typedef Stream next_layer_type;
	typedef Serialize serialize_type;
	typedef rpc::detail::packet<async_handler> packet;
	typedef typename packet::list_type packet_list;

	receive_state m_state;

	async_asio_stream(asio::io_service& ios, serialize_type serialize = serialize_type())
		: m_socket(ios)
		, m_serialize(serialize)
		, m_state(rs_init)
//		, m_max_payload_size(65536)
	//	, m_decoder(m_recv_header)
	{
	}

	~async_asio_stream()
	{
		m_send_queue.clear_and_dispose(boost::checked_deleter<packet>());
	}

	next_layer_type& next_layer()
	{
		return m_socket;
	}

	void async_send(const header_type& header, buffer_type& payload, const async_handler& handler = async_handler() )
	{
		std::auto_ptr<packet> p(new packet(header, payload, handler, m_serialize));
		bool do_send = m_send_queue.empty();
		m_send_queue.push_back(*p.release());
		if(do_send)
		{
			priv_send_one();
		}
	}

	void start()
	{
		m_state = rs_control_data;
		priv_recv();
	}

	uint32_t max_payload_size() const
	{
		return m_max_payload_size;
	}

	uint32_t max_payload_size(uint32_t n)
	{
		std::swap(m_max_payload_size, n);
		return n;
	}

private:

	void priv_recv()
	{
		m_recv_buffer.reset2();
		m_socket.async_read_some(asio::mutable_buffers_1(m_recv_buffer.prepare()),
			boost::bind(&async_asio_stream::priv_handle_recv, static_cast<Derived*>(this)->shared_from_this(), _1, _2));
	}

	void priv_handle_recv(const system::error_code& ec, std::size_t size)
	{
		if(ec)
		{
			static_cast<Derived*>(this)->receive_error(ec);
			return;
		}
		bool receive_again = false;
		m_recv_buffer.commit(size);
		while(!m_recv_buffer.empty())
		{
			if(m_state == rs_control_data)
			{
				boost::tribool result = m_control_decoder.process(m_recv_buffer.pop());
				if(result == true)
				{
					m_control_decoder.init_buffers(m_header_buffer, m_payload_buffer);
					std::size_t remaining = m_recv_buffer.flush(std::back_inserter(m_header_buffer), m_control_decoder.header_size());
					if(remaining != 0)
					{
						m_recv_buffer.flush(std::back_inserter(m_payload_buffer), m_control_decoder.payload_size());
						if(m_payload_buffer.size() == m_control_decoder.payload_size())
						{
							receive_again = this->priv_dispatch();
							m_control_decoder.reset();
						}
						else
						{
							m_state = rs_payload_buffer;
						}

					}
					else
					{
						m_state = rs_header_buffer;
					}
				}
				else if(result == false)
				{
					static_cast<Derived*>(this)->receive_error(boost::system::error_code()); // TODO: new error code
					return;
				}
			}
		}
		if(receive_again)
		{
			this->priv_recv();
		}
	}

	bool priv_dispatch()
	{
		Header header;
		Serialize::reader(m_serialize, m_header_buffer)(header, rpc::tags::default_());
		return static_cast<Derived*>(this)->receive(header, m_payload_buffer);

	}

	void priv_send_one()
	{
		const packet& p = m_send_queue.front();
		asio::async_write(m_socket,
			p.to_buffers(), 
			boost::bind(&async_asio_stream::priv_handle_send, static_cast<Derived*>(this)->shared_from_this(), // TODO: Remove dep. on bind
			_1));

	}

	void priv_handle_send(const system::error_code& err)
	{
		std::auto_ptr<packet> p(&m_send_queue.front());
		m_send_queue.pop_front();
		bool do_send = !m_send_queue.empty(); // Check queue before invoking handler, as it may call async_send
		if(p->m_handler) // The handler is optional
		{
			p->m_handler(static_cast<Derived*>(this)->shared_from_this(), p->m_payload, err);
		}
		if(do_send)
		{
			priv_send_one();
		}
	}

	Stream	m_socket;
	serialize_type m_serialize;
	packet_list m_send_queue;
	std::vector<char> m_recv_payload;
	buffer_type m_header_buffer;
	buffer_type m_payload_buffer;
	header_type m_recv_header;
	rpc::detail::control_data::decoder m_control_decoder;
/*	typename headerx_type::decoder m_decoder;
	uint32_t m_max_payload_size;*/
	rpc::detail::receive_buffer<64> m_recv_buffer;
};

/*
			{
				std::size_t payload_size = m_decoder.payload_size();
				if(payload_size == 0)
				{
					recv_new_header = priv_dispatch(system::error_code());
					m_recv_payload.clear();
					m_decoder.reset();
				}
				else if(payload_size < m_max_payload_size)
				{
					m_recv_payload.resize(payload_size);
					std::size_t available_payload = m_recv_buffer.flush(&m_recv_payload[0], payload_size);
					if(std::size_t remaining_payload = (payload_size - available_payload))
					{
						asio::async_read(m_socket, // Read directly into payload buffer
							asio::buffer(&m_recv_payload[0] + available_payload, remaining_payload),
							boost::bind(&async_stream::priv_handle_recv_payload, static_cast<Derived*>(this)->shared_from_this(), _1, _2));
						recv_new_header = false;
						break;
					}
					else
					{
						recv_new_header = priv_dispatch(boost::system::error_code());
						m_recv_payload.clear();
						m_decoder.reset();
					}
				}
				else
				{
					// ERROR, payload size overflow
					priv_dispatch(boost::system::error_code()); // TODO: New error code here
				}
			}*/


template<class StreamProtocol>
class stream_connector
{
	template<class Handler>
	struct async_iterator
	{
		typename StreamProtocol::socket& m_socket;
		typename StreamProtocol::resolver::iterator m_endpoint_itr;
		Handler m_handler;

		async_iterator(typename StreamProtocol::socket& sock, Handler h)
			: m_socket(sock)
			, m_endpoint_itr()
			, m_handler(h)
		{

		}

		// Handle connect
		void operator()(const system::error_code& ec)
		{
			if(!ec)
			{
				m_handler(ec);
			}
			else
			{
				m_socket.close();
				if(++m_endpoint_itr != typename StreamProtocol::resolver::iterator())
				{
					m_socket.async_connect(*m_endpoint_itr, *this);
				}
				else
				{
					m_handler(ec);
				}
			}
		}

		// Handle resolve
		void operator()(const system::error_code& ec, typename StreamProtocol::resolver::iterator endpoint_itr)
		{
			if(!ec)
			{
				m_endpoint_itr = endpoint_itr;
				m_socket.async_connect(*m_endpoint_itr, *this);
			}
			else
			{
				m_handler(ec);
			}
		}

	private:
		async_iterator& operator=(const async_iterator&);
	};

public:
	typename StreamProtocol::resolver::iterator m_endpoint_itr;
	shared_ptr<typename StreamProtocol::resolver> m_resolver;

	stream_connector(asio::io_service& ios)
		: m_endpoint_itr()
		, m_resolver(new typename StreamProtocol::resolver(ios))
	{
	}

	template<class Handler>
	void async_connect(typename StreamProtocol::socket& sock, const std::string& host_name, const std::string& service_name, Handler h)
	{
		m_resolver->async_resolve(typename StreamProtocol::resolver::query(host_name, service_name),
			async_iterator<Handler>(sock, h));
	}

};

template<class StreamProtocol>
class stream_acceptor
{
	template<class ConnectionPtr>
	struct accept_handler
	{
		accept_handler(ConnectionPtr ptr)	:
		m_ptr(ptr)
		{}

		void operator()(const system::error_code& ec)
		{
			if(!ec)
				m_ptr->start();
			else
				m_ptr->accept_error(ec);
		}

	private:
		ConnectionPtr m_ptr;

	};

public:
	stream_acceptor(
		asio::io_service& ios,
		const std::string& address,
		const std::string& service_name) :
		acceptor_(ios)
	{
		typename StreamProtocol::resolver resolver(ios);
		typename StreamProtocol::resolver::query query(address, service_name);
		typename StreamProtocol::endpoint endpoint = *resolver.resolve(query);
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(typename StreamProtocol::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();
	}

	template<class Handler>
	void async_accept(typename StreamProtocol::socket& sock, Handler h)
	{
		acceptor_.async_accept(sock, h);
	}

private:
	typename StreamProtocol::acceptor acceptor_;
};

}
}
}

#endif
