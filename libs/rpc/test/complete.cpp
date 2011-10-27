/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/config.hpp>

#ifdef BOOST_WINDOWS
#define _WIN32_WINNT 0x0501 // To avoid warnings when including asio
#endif

#define BOOST_DATE_TIME_NO_LIB

#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/rpc/core/async_remote.hpp>
#include <boost/rpc/core/local.hpp>
#include <boost/rpc/service/async_tcp.hpp>
#include <boost/variant/get.hpp>

#include <boost/detail/lightweight_test.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/function/function2.hpp>


#include <map>

#include "common/signatures.hpp"

const int N_ROUNDTRIPS = 10;

namespace rpc = boost::rpc;
namespace asio = boost::asio;
using boost::system::error_code;

struct bitwise
{
	typedef std::vector<char> input_type;
	typedef std::vector<char> output_type;
	struct reader : rpc::protocol::bitwise_reader<>
	{
		reader(bitwise, std::vector<char> & v) : rpc::protocol::bitwise_reader<>(v) {}
	};
	struct writer : rpc::protocol::bitwise_writer<>
	{
		writer(bitwise, std::vector<char> & v) : rpc::protocol::bitwise_writer<>(v) {}
	};
};

template<class FunctionID, class CallID = char>
struct commands
{
	typedef CallID call_id_type;
	struct call
	{
		CallID call_id;
		FunctionID function_id;
		call()
			: call_id()
			, function_id()
		{}

		template<class Archive>
		void serialize(Archive& ar, unsigned int)
		{
			ar & call_id;
			ar & function_id;
		}
	};

	struct result
	{
		CallID call_id;
		result()
			: call_id()
		{}

		result(char call_id)
			: call_id(call_id)
		{}

		template<class Archive>
		void serialize(Archive& ar, unsigned int)
		{
			ar & call_id;
		}
	};

	struct result_exception
	{
		CallID call_id;
		result_exception()
			: call_id()
		{}

		result_exception(char call_id)
			: call_id(call_id)
		{}

		template<class Archive>
		void serialize(Archive& ar, unsigned int)
		{
			ar & call_id;
		}
	};

};

typedef commands<std::string> commands_t;
typedef boost::variant<commands_t::call, commands_t::result, commands_t::result_exception> header;

template<class FunctionID>
struct library
{
	typedef std::vector<char> buffer_type;
	typedef boost::function<void(buffer_type&, buffer_type&)> function_type;
	typedef std::map<FunctionID, function_type> function_map;

	void add(const typename function_map::value_type& f)
	{
		m_functions.insert(f);
	}

	bool call(const FunctionID& f, buffer_type& in, buffer_type& out)
	{
		typename function_map::iterator itr = m_functions.find(f);
		if(itr != m_functions.end())
		{
			itr->second(in, out);
			return true;
		}
		return false;
	}

	function_map m_functions;
};

typedef library<std::string> library_t;

typedef boost::function<void(std::vector<char>& data, const boost::system::error_code&)> result_handler;

template<class Library>
struct basic_connection
	: boost::rpc::service::async_asio_stream<
		basic_connection<Library>, header, asio::ip::tcp::socket, bitwise>
	, boost::enable_shared_from_this<basic_connection<Library > >
{
	typedef typename basic_connection<Library>::async_stream_base async_stream_base;
	typedef std::vector<char> buffer_type;
	struct command_visitor : boost::static_visitor<bool>
	{
		command_visitor(basic_connection& c, buffer_type& buffer)
			: m_connection(c)
			, m_buffer(buffer)
		{}
		template<class Command>
		bool operator()(const Command& c) const
		{
			return m_connection.command(c, m_buffer);
		}
		basic_connection& m_connection;
		buffer_type& m_buffer;
	};

	basic_connection(asio::io_service& ios, Library& library, const char* debug_name)
		: async_stream_base(ios)
		, m_library(library)
		, m_debug_name(debug_name)
	{}

	struct send_handler
	{
		send_handler(basic_connection& c, commands_t::call_id_type id)
			: m_connection(c)
			, m_id(id)
		{}

		void operator()(buffer_type& buffer, const error_code& ec)
		{
			if(ec)
			{
				buffer.clear();
				m_connection.invoke_result_handler(m_id, buffer, ec);
			}
		}
		basic_connection& m_connection;
		commands_t::call_id_type m_id;
	};

	bool receive(const header& input_header, std::vector<char>& buffer)
	{
		return boost::apply_visitor(command_visitor(*this, buffer), input_header);
	}

	void receive_error(boost::system::error_code)
	{

	}

	bool command(const commands_t::call& c, buffer_type& input_buffer)
	{
		std::vector<char> output_buffer;
		try
		{
			if(m_library.call(c.function_id, input_buffer, output_buffer))
			{
				this->async_send(commands_t::result(c.call_id), output_buffer);
			}
			else
			{
			}
		}
		catch(rpc_test::quit_exception&)
		{
			output_buffer.clear(); // reuse the buffer
			bitwise::writer write(bitwise(), output_buffer);
			this->async_send(commands_t::result_exception(c.call_id), output_buffer);
			return false;
		}
		return true;
	}
	bool command(const commands_t::result& r, buffer_type& buffer)
	{
		this->invoke_result_handler(r.call_id, buffer, error_code());
		return true;
	}

	bool command(const commands_t::result_exception& r, buffer_type& buffer)
	{
		try
		{
			this->invoke_result_handler(r.call_id, buffer, rpc::remote_exception);
		}
		catch(rpc_test::quit_exception&)
		{
			return false;
		}
		return true;
	}

	void invoke_result_handler(commands_t::call_id_type id, buffer_type& buffer, const error_code& ec)
	{
		result_handler_map::iterator handler_itr = m_result_handlers.find(id);
		if(handler_itr != m_result_handlers.end())
		{
			result_handler handler;
			handler_itr->second.swap(handler);
			m_result_handlers.erase(handler_itr);
			handler(buffer, ec);
		}
		else
		{
			// Unknown call_id
		}
	}

	void async_call(const std::string& id, std::vector<char>& data, const result_handler& handler)
	{
		commands_t::call call;
		if(handler)
		{
			call.call_id = m_call_id_enum++;
			if(m_result_handlers.insert(result_handler_map::value_type(call.call_id, handler)).second == false)
			{
				buffer_type empty;
				handler(empty, rpc::internal_error);
				return;
			}
		}
		else
		{
			call.call_id = 0;
		}
		call.function_id = id;
		send_handler x(*this, call.call_id);
		this->async_send(call, data);
	}

	typedef std::map<commands_t::call_id_type, result_handler> result_handler_map;
	result_handler_map m_result_handlers;
	Library& m_library;
	char m_call_id_enum;
	const char* m_debug_name;
};

typedef basic_connection<library_t> connection;
typedef boost::shared_ptr<connection> connection_ptr;

void rpc_async_call(connection_ptr c, const std::string& id, std::vector<char>& data)
{
	c->async_call(id, data, result_handler());
}

void rpc_async_call(connection_ptr c, const std::string& id, std::vector<char>& data, const result_handler& handler)
{
	c->async_call(id, data, handler);
}

void on_sig1(char result, const error_code& ec)
{
	BOOST_TEST(!ec);
	BOOST_TEST_EQ((int)result, 53);
}

void do_quit(error_code ec)
{
	BOOST_TEST_EQ(ec, rpc::remote_exception);
	throw rpc_test::quit_exception();
}

void on_increment(connection_ptr client, int result, error_code ec)
{
	BOOST_TEST(!ec);
	if(!ec)
	{
		rpc::async_remote<bitwise> async_remote;
		if(result < N_ROUNDTRIPS)
		{
			async_remote(rpc_test::increment, client, boost::bind(&on_increment, client, _1, _2))(1);
		}
		else
		{
			async_remote(rpc_test::quit, client, &do_quit)();
		}
	}
}

void run_client(boost::system::error_code ec, connection_ptr client)
{
	if(!ec)
	{
		client->start();
		rpc::async_remote<bitwise> async_remote;
		async_remote(rpc_test::void_char, client, &on_sig1)(5);
		on_increment(client, 0, error_code());
	}
	else
	{
		BOOST_TEST(!ec);
	}
}

void run_server(boost::system::error_code ec, connection_ptr server)
{
	if(!ec)
	{
		server->start();
	}
}

void test1(char in, char& out)
{
	BOOST_TEST(in == 5);
	out = 53;
}

namespace server
{
	int m_counter = 0;
	int increment(int i)
	{
		return m_counter += i;
	}

	void quit()
	{
		throw rpc_test::quit_exception();
	}
}

int main()
{
	asio::io_service ios;
	library_t client_lib;
	library_t server_lib;

	rpc::local<bitwise> local; 

	server_lib.add(local(rpc_test::void_char, &test1));
	server_lib.add(local(rpc_test::increment, &server::increment));
	server_lib.add(local(rpc_test::quit, &server::quit));

	connection_ptr client(new connection(ios, client_lib, "client"));
	connection_ptr server(new connection(ios, server_lib, "server"));

	rpc::service::stream_connector<asio::ip::tcp> connector(ios);
	rpc::service::stream_acceptor<asio::ip::tcp> acceptor(ios, "127.0.0.1", "10001");
	acceptor.async_accept(server->next_layer(), boost::bind(&run_server, _1, server));
	connector.async_connect(client->next_layer(), "127.0.0.1", "10001", boost::bind(&run_client, _1, client));

	ios.run();
	BOOST_TEST(server::m_counter == N_ROUNDTRIPS);
	
	return boost::report_errors();
}
