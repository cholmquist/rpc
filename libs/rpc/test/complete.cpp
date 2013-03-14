/*=============================================================================
    Copyright (c) 2012 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/config.hpp>

#ifdef BOOST_WINDOWS
#define _WIN32_WINNT 0x0501 // To avoid warnings when including asio
#endif

#define BOOST_DATE_TIME_NO_LIB
#define BOOST_ALL_NO_LIB

#include <boost/rpc/service/async_stream_connection.hpp>
#include <boost/rpc/service/async_tcp.hpp>
#include <boost/rpc/service/async_commander.hpp>

#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/rpc/core/async_remote.hpp>
#include <boost/rpc/core/remote.hpp>
#include <boost/rpc/core/remote.hpp>
#include <boost/rpc/core/local.hpp>

#include <boost/detail/lightweight_test.hpp>

#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>


#include "common/call_over_async.hpp"

#include <map>

#include "common/signatures.hpp"

const int N_ROUNDTRIPS = 10;
const char RESULT_1 = 'B';

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
 
typedef rpc::commands<std::string, char> commands_t;
typedef std::vector<char> buffer_type;
typedef boost::function<void(buffer_type&, buffer_type&)> local_function_type;
typedef std::map<commands_t::function_id_type, local_function_type> function_map;

class connection
  : public rpc::async_stream<connection, commands_t::variant_type, bitwise>
  , public rpc::async_tcp<connection>
  , public rpc::async_commander<connection, commands_t, function_map>
  , public boost::enable_shared_from_this<connection>
{
public:
    
    typedef boost::shared_ptr<connection> shared_this_ptr;
    
  connection(asio::io_service& ios, function_map& function_map)
    : async_tcp_t(ios)
    , async_commander_t(function_map)
    {
    }
    void receive_error(boost::system::error_code)
    {

    }
    template<class Command>
    void command(const Command& c, std::vector<char>& buffer)
    {
      return async_commander_t::command(c, buffer);
    }
    

    void command(const commands_t::result_exception& r, std::vector<char>& buffer)
    {
      this->invoke_result_handler(r.call_id, buffer, rpc::remote_exception);
    }

      virtual void connected(error_code ec) = 0;
      virtual void unconnected_test() = 0;
  
  
};

typedef boost::shared_ptr<connection> connection_ptr;


void rpc_async_call(connection_ptr c, const std::string& id, std::vector<char>& data)
{
	c->async_call(id, data, connection::result_handler());
}

void rpc_async_call(connection_ptr c, const std::string& id, std::vector<char>& data, const connection::result_handler& handler)
{
	c->async_call(id, data, handler);
}

void rpc_call(connection_ptr c, const std::string& id, std::vector<char>& in, std::vector<char>& out)
{
	boost::system::error_code ec;
	rpc::call_over_async handler;
	c->async_call(id, in, handler);
	handler.get(out, ec);
}

void do_quit(error_code ec)
{
	BOOST_TEST_EQ(ec, rpc::remote_exception);
	throw rpc::abort_exception();
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

void test1(char in, char& out)
{
	BOOST_TEST_EQ(in, 5);
	out = RESULT_1;
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
		throw rpc::abort_exception();
	}
}

class server_connection : public connection
{
public:
      server_connection(asio::io_service& ios, function_map_type &fm)
      : connection(ios, fm)
      {
      }
      
      virtual void connected(error_code ec)
      {
	if(!ec)
	{
		this->start();
	}
      }
        virtual void unconnected_test() {}

};

class client_connection : public connection
{
public:
      std::auto_ptr<boost::thread> m_thread;
      boost::barrier m_barrier;
      bool m_unconnected_test_called;
  
      client_connection(asio::io_service& ios, function_map_type &fm)
      : connection(ios, fm)
      , m_unconnected_test_called(false)
      , m_barrier(2)
      {
	
      }
      
      virtual void connected(error_code ec)
      {
	if(!ec)
	{
		this->start();
		m_thread.reset(new boost::thread(&client_connection::thread_entry, this));
		m_barrier.wait();
		rpc_test::f1::async_call(bitwise(), shared_from_this());
		on_increment(shared_from_this(), 0, error_code());
	}
	else
	{
		BOOST_TEST(!ec);
	}
      }
      virtual void unconnected_test()
      {
		rpc::async_remote<bitwise> async_remote;
 		async_remote(rpc_test::f1::sig, shared_from_this(), boost::bind(&client_connection::expect_error, this, _1, _2)  )(5);
      }
            
      void thread_entry()
      {
	m_barrier.wait();
	rpc_test::f1::call(bitwise(), shared_from_this());
      }
            
      void expect_error(char result, const error_code& ec)
      {
	      m_unconnected_test_called = true;
	      BOOST_TEST(result == 0);
	      BOOST_TEST(ec);
      }
  
};

int main()
{
	asio::io_service ios;
	function_map client_lib;
	function_map server_lib;
	rpc::local<bitwise> local; 

	rpc_test::register_all(local, server_lib);
	//server_lib.insert(local(rpc_test::f, &test1));
	server_lib.insert(local(rpc_test::increment, &server::increment));
	server_lib.insert(local(rpc_test::quit, &server::quit));

	connection_ptr client(new client_connection(ios, client_lib));

	client->unconnected_test();
	ios.run();
	ios.reset();
	
	connection_ptr server(new server_connection(ios, server_lib));
	connection::acceptor acceptor(ios, "127.0.0.1", "10001");
	connection::connector connector(ios, "127.0.0.1", "10001");

	acceptor.async_accept(server);
	connector.async_connect(client);

	ios.run();
	BOOST_TEST(server::m_counter == N_ROUNDTRIPS);
	rpc_test::verify_all_called();
	return boost::report_errors();
}

