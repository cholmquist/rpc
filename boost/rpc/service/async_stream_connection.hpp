#ifndef BOOST_RPC_ASYNC_STREAM_CONNECTION_HPP
#define BOOST_RPC_ASYNC_STREAM_CONNECTION_HPP

#include <boost/rpc/service/stream_header.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/rpc/core/tags.hpp> // TODO: remove
#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/function/function2.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/buffer.hpp>
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
				typename Serialize::writer(serialize, m_header)(header, rpc::tags::default_());
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

template<class Derived, class Header, class Serialize>
class async_stream
{
public:
	typedef async_stream<Derived, Header, Serialize> async_stream_t;
	typedef std::vector<char> buffer_type;
	typedef boost::function<void(buffer_type&, system::error_code)> async_handler;
	typedef Header header_type;
	typedef Serialize serialize_type;
	typedef rpc::detail::packet<async_handler> packet;
	typedef typename packet::list_type packet_list;

	async_stream(std::size_t receive_buffer_size = 64, serialize_type serialize = serialize_type())
		: m_serialize(serialize)
		, m_recv_buffer(receive_buffer_size)
	{
	}

	~async_stream()
	{
		m_send_queue.clear_and_dispose(boost::checked_deleter<packet>());
	}

	void async_send_package(const header_type& header, buffer_type& payload, const async_handler& handler = async_handler())
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
		priv_recv();
	}

	void completed_async_write(const system::error_code& ec,std::size_t size)
	{
		std::auto_ptr<packet> p(&m_send_queue.front());
		m_send_queue.pop_front();
				
		if(!ec)
		{
			BOOST_ASSERT(asio::buffer_size(p->to_buffers()) == size);
		}
		else
		{
			header_type header;
			typename Serialize::reader reader(Serialize(), p->m_header);
			reader(header, tags::default_());
			(static_cast<Derived*>(this))->send_error(header, p->m_payload, ec);
		}

		
		bool do_send = !m_send_queue.empty(); // Check queue before invoking handler, as it may call async_send
		if(p->m_handler) // The handler is optional
		{
			p->m_handler(p->m_payload, ec);
		}
		if(do_send)
		{
			priv_send_one();
		}
	}
  
	void completed_async_read_some(const system::error_code& ec, std::size_t size)
	{
		if(ec)
		{
			static_cast<Derived*>(this)->receive_error(ec);
			return;
		}
		m_recv_buffer.commit(size);
		while(!m_recv_buffer.empty())
		{
			if(m_control_decoder.is_done())
			{
			    if(m_header_buffer.size() != m_control_decoder.header_size())
			    {
				m_recv_buffer.flush(std::back_inserter(m_header_buffer), m_control_decoder.header_size() - m_header_buffer.size());
			    }
			    else if(m_payload_buffer.size() != m_control_decoder.payload_size())
			    {
				m_recv_buffer.flush(std::back_inserter(m_payload_buffer), m_control_decoder.payload_size() - m_payload_buffer.size());
			    }
			    if(m_header_buffer.size() == m_control_decoder.header_size() &&
			      m_payload_buffer.size() == m_control_decoder.payload_size())
			    {
				Header header;
				{
				  typename Serialize::reader reader(m_serialize, m_header_buffer);
				  reader(header, rpc::tags::default_());
				}
				m_control_decoder.reset();
				try
				{
				  static_cast<Derived*>(this)->receive(header, m_payload_buffer);
				}
				catch(abort_exception&)
				{
				  return;
				}
			    }
			}
			else
			{
				boost::tribool result = m_control_decoder.process(m_recv_buffer.pop());
				if(result == true)
				{
				      m_header_buffer.clear();
				      m_payload_buffer.clear();
				}
				else if(result == false)
				{
					static_cast<Derived*>(this)->receive_error(boost::system::error_code()); // TODO: new error code
					return;
				}
			}
		}
		this->priv_recv();
	}

  
private:

	void priv_recv()
	{
		m_recv_buffer.reset2();
		static_cast<Derived*>(this)->async_read_some(asio::mutable_buffers_1(m_recv_buffer.prepare()));
	}

	void priv_send_one()
	{
		const packet& p = m_send_queue.front();
		static_cast<Derived*>(this)->async_write(p.to_buffers());

	}

	serialize_type m_serialize;
	packet_list m_send_queue;
	std::vector<char> m_recv_payload;
	buffer_type m_header_buffer;
	buffer_type m_payload_buffer;
	header_type m_recv_header;
	rpc::detail::control_data::decoder m_control_decoder;
	rpc::detail::receive_buffer m_recv_buffer;
};


}
}

#endif
