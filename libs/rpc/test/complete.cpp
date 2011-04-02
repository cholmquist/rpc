/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/config.hpp>

#ifdef BOOST_WINDOWS
#define _WIN32_WINNT 0x0501 // To avoid warnings when including asio
#endif

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

namespace rpc = boost::rpc;
namespace asio = boost::asio;
using boost::system::error_code;

template<class FunctionID, class CallID = char>
struct commands
{
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

	enum type_id
	{
		tid_call,
		tid_result,
	};

};

typedef commands<std::string> commands_t;
typedef boost::variant<commands_t::call, commands_t::result> header;

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

	void call(const FunctionID& f, buffer_type& in, buffer_type& out)
	{
		function_map::iterator itr = m_functions.find(f);
		if(itr != m_functions.end())
		{
			itr->second(in, out);
		}
	}

	function_map m_functions;
};

typedef library<std::string> library_t;

typedef const boost::function<void(std::vector<char>& data, const boost::system::error_code&)> async_call_result;

template<class Library>
struct basic_connection
	: boost::rpc::service::async_asio_stream<
		basic_connection<Library>, header, asio::ip::tcp::socket, rpc::protocol::bitwise>
	, boost::enable_shared_from_this<basic_connection<Library > >
{
	basic_connection(asio::io_service& ios, Library& library, const char* debug_name)
		: async_stream_base(ios)
		, m_library(library)
		, m_debug_name(debug_name)
	{}

	bool receive(const header& input_header, std::vector<char>& input_buffer)
	{
		if(const commands_t::call* c = boost::get<commands_t::call>(&input_header))
		{
			std::vector<char> output_buffer;
			m_library.call(c->function_id, input_buffer, output_buffer);
			async_send(commands_t::result(c->call_id), output_buffer);
		}
		else if(const commands_t::result* r = boost::get<commands_t::result>(&input_header))
		{
			int a = 0;
		}
		return true;

	}
	void receive_error(boost::system::error_code)
	{

	}

	void async_call(const std::string& id, std::vector<char>& data, const async_call_result& handler)
	{
		commands_t::call call;
		if(handler)
		{
			call.call_id = m_call_id_enum++;
		}
		else
		{
			call.call_id = 0;
		}
		call.function_id = id;
		std::size_t x = data.size();

		async_send(call, data, handler);
	}

	Library& m_library;
	char m_call_id_enum;
	const char* m_debug_name;
};

typedef basic_connection<library_t> connection;
typedef boost::shared_ptr<connection> connection_ptr;

void rpc_async_call(connection_ptr c, const std::string& id, std::vector<char>& data)
{
	c->async_call(id, data, async_call_result());
}


void rpc_async_call(connection_ptr c, const std::string& id, std::vector<char>& data, const async_call_result& handler)
{
	// TODO: Make handler a template, wrap it in something that checks for errors, if no errors inserts the actual handler in the receive map
	c->async_call(id, data, handler);
}

void on_sig1(char result, const error_code& ec)
{
	BOOST_TEST(!ec);
	BOOST_TEST_EQ(result, 53);
}

void run_client(boost::system::error_code ec, connection_ptr client)
{
	if(!ec)
	{
		client->start();
		rpc::async_remote<rpc::protocol::bitwise> async_remote;
//		async_remote(rpc_test::void_char, client)(5);
		async_remote(rpc_test::void_char, client, &on_sig1)(5);
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

int main()
{
	asio::io_service ios;
	library_t client_lib;
	library_t server_lib;

	rpc::local<rpc::protocol::bitwise> local;

	server_lib.add(local(rpc_test::void_char, &test1));

	connection_ptr client(new connection(ios, client_lib, "client"));
	connection_ptr server(new connection(ios, server_lib, "server"));

	rpc::service::stream_connector<asio::ip::tcp> connector(ios);
	rpc::service::stream_acceptor<asio::ip::tcp> acceptor(ios, "127.0.0.1", "10000");
	acceptor.async_accept(server->next_layer(), boost::bind(&run_server, _1, server));
	connector.async_connect(client->next_layer(), "127.0.0.1", "10000", boost::bind(&run_client, _1, client));

	ios.run();
	
	return boost::report_errors();
}
