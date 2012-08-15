//#pragma warning (4 : 4365)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef BOOST_DATE_TIME_NO_LIB
#define BOOST_DATE_TIME_NO_LIB
#endif

#include "../common/async_connection.hpp"
#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/bind.hpp>


namespace rpc=boost::rpc;
namespace asio=boost::asio;

enum tests
{
	test_client_connect,
	test_server_accept,
	test_send_header_a,
	test_recv_header_a,
	test_send_header_b_with_data,
	test_recv_header_b_with_data,
	num_tests,
};

static char test_results[num_tests] = {false};
const boost::array<char, 4> data_header_b = {1, 2, 3, 4};
static const char value_header_a = 101;
static const char value_header_b = 102;

struct Serialize
{
	struct reader : rpc::protocol::bitwise_reader<>
	{
		reader(Serialize, std::vector<char> & v) : rpc::protocol::bitwise_reader<>(v) {}
	};
	struct writer : rpc::protocol::bitwise_writer<>
	{
		writer(Serialize, std::vector<char> & v) : rpc::protocol::bitwise_writer<>(v) {}
	};
};

struct header_a : boost::array<char, 1>
{
	header_a(char value = 0)
	{
		data()[0] = value;
	}

	char value() const
	{
		return data()[0];
	}
};

struct header_b : boost::array<char, 2>
{
	static const int static_size = 2;
	header_b(char value = 0)
	{
		data()[0] = value;
		data()[1] = static_cast<char>(126);
	}

	char value() const
	{
		return data()[0];
	}
};

namespace boost{ namespace rpc{ namespace traits{

template<>
struct is_array<header_a> : boost::true_type {};

template< >
struct is_array<header_b> : boost::true_type {};

}}}

typedef boost::variant<header_a, header_b> header_variant;


struct client : public rpc_test::connection<header_variant, Serialize>
{
	client(asio::io_service& ios, std::size_t recvsize) : connection(ios, recvsize) {}

	virtual void receive_error(boost::system::error_code ec)
	{
	  BOOST_ERROR(ec.message().c_str());
	}
	
	bool receive(const header_variant&, std::vector<char>&)
	{
	  return true;
	}

};

struct server : rpc_test::connection<header_variant, Serialize>
{
	server(asio::io_service& ios, std::size_t recvsize) : connection(ios, recvsize) {}

	virtual void receive_error(boost::system::error_code ec)
	{
	  BOOST_ERROR(ec.message().c_str());
	}

	bool receive(const header_variant& header, std::vector<char>& data)
	{
		if(header.which() == 0) // header_a
		{
			test_results[test_recv_header_a] = true;
			BOOST_TEST(boost::get<header_a>(header).value() == value_header_a);
			BOOST_TEST(data.empty());
		}
		else if(header.which() == 1) // header_b
		{
			test_results[test_recv_header_b_with_data] = true;
			BOOST_TEST(boost::get<header_b>(header).value() == value_header_b);
			BOOST_TEST(data == std::vector<char>(data_header_b.begin(), data_header_b.end()));
			return false;
		}
		return true;
	}

};

typedef boost::shared_ptr<client> client_ptr;
typedef boost::shared_ptr<server> server_ptr;

client_ptr g_client;
server_ptr g_server;

void on_accept(boost::system::error_code ec)
{
	if(!ec)
	{
		test_results[test_server_accept] = true;
		g_server->start();
	}
}

void on_send_header_b(std::vector<char>&, const boost::system::error_code& ec)
{
	if(!ec)
	{
		test_results[test_send_header_b_with_data] = true;
	}
}

void on_send_header_a(std::vector<char>&, const boost::system::error_code& ec)
{
	if(!ec)
	{
		test_results[test_send_header_a] = true;
		std::vector<char> payload(data_header_b.begin(), data_header_b.end());
		g_client->async_send(header_b(value_header_b), payload, &on_send_header_b);
	}
}

void on_connect(boost::system::error_code ec)
{
	if(!ec)
	{
		test_results[test_client_connect] = true;
		std::vector<char> payload;
		g_client->async_send(header_a(value_header_a), payload, &on_send_header_a);
	}
}

int main()
{
	for(int i = 0; i < 1; ++i)
	{
	  for(int j = 0; j < num_tests; j++)
	  {
	    test_results[j] = false;
	  }
	  boost::asio::io_service ios;
	  g_client.reset(new client(ios, 64));
	  g_server.reset(new server(ios, 64));
	  rpc_test::stream_connector<asio::ip::tcp> connector(ios);
	  rpc_test::stream_acceptor<asio::ip::tcp> acceptor(ios, "127.0.0.1", "10002");
	  acceptor.async_accept(g_server->socket(), &on_accept);
	  connector.async_connect(g_client->socket(), "127.0.0.1", "10002", &on_connect);
	  ios.run();
	  BOOST_TEST(test_results[test_client_connect]);
	  BOOST_TEST(test_results[test_server_accept]);
	  BOOST_TEST(test_results[test_send_header_a]);
	  BOOST_TEST(test_results[test_recv_header_a]);
	  BOOST_TEST(test_results[test_send_header_b_with_data]);
	  BOOST_TEST(test_results[test_recv_header_b_with_data]);
	  g_client.reset();
	  g_server.reset();
	}
	return boost::report_errors();
}
