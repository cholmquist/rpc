#ifndef BOOST_RPC_CORE_COORDINATOR_HPP
#define BOOST_RPC_CORE_COORDINATOR_HPP

namespace boost{ namespace rpc{

struct async_coordinator
{
	typedef boost::uint8_t signature_id;
	typedef boost::uint8_t packet_id;

	struct call_header
	{
		static const int static_size = 2;

		call_header(boost::uint8_t signature_id = 0)
		{
			m_data[0] = signature_id;
			m_data[1] = 0;
		}

		call_header(boost::uint8_t signature_id, boost::uint8_t pid)
		{
			m_data[0] = signature_id;
			m_data[1] = pid;
		}

		boost::uint8_t signature_id() const { return m_data[0]; }

		boost::uint8_t packet_id() const { return m_data[1]; }

		boost::uint8_t* data() { return m_data; }
		const boost::uint8_t* data() const { return m_data; }

	private:

		boost::uint8_t m_data[2];
	};

	struct result_header
	{
		static const int static_size = 1;

		explicit result_header(boost::uint8_t packet_id = 0)
			: m_data(packet_id)
		{
		}

		boost::uint8_t packet_id() const { return m_data; }

		boost::uint8_t* data() { return &m_data; }
		const boost::uint8_t* data() const { return &m_data; }

	private:
		boost::uint8_t m_data;
	};

	struct exception_header
	{
		static const int static_size = 2;

		exception_header(boost::uint8_t packet_id = 0, boost::uint8_t exception_id = 0)
		{
			m_data[0] = packet_id;
			m_data[1] = exception_id;
		}

		boost::uint8_t packet_id() const { return m_data[0]; }
		boost::uint8_t exception_id() const { return m_data[1]; }

		boost::uint8_t* data() { return m_data; }
		const boost::uint8_t* data() const { return m_data; }

	private:

		boost::uint8_t m_data[2];
	};

	template<class Handler, class Context>
	struct result_handler
	{
		result_handler(Handler h)
			: h(h)
		{}
/*
		void operator()(connection_ptr p, std::vector<char>& out, boost::system::error_code ec)
		{
			h(out);
		}*/

		Handler h;
		packet_id pid;
	};

	struct async_call
	{
		template<class Handler>
		struct send_handler
		{
			send_handler(Handler h, packet_id pid)
				: h(h)
				, pid(pid)
			{
			}
/*
			void operator()(connection_ptr p, std::vector<char>&, boost::system::error_code ec)
			{
				p->async_receive(pid, receive_handler<Handler>(h));
			}
*/
			Handler h;
			packet_id pid;
		};

		template<class ServicePtr>
		async_call(ServicePtr srv)
			: service(srv)
		{

		}

		template<class F>
		void operator()(boost::uint8_t signature_id, std::vector<char>& input, F f)
		{
			send_handler<F> handler(f, m_connection->new_pid());
			async_call_header header(signature_id, handler.pid.value());
			m_connection->async_send(header, input, handler);
		}

		void operator()(boost::uint8_t signature_id, std::vector<char>& input)
		{
			async_call_header header(signature_id);
			m_connection->async_send(async_call_header(0, signature_id), input);
		}

		ServicePtr service;
	};



};

}}

#endif
