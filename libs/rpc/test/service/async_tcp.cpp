//#pragma warning (4 : 4365)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef BOOST_DATE_TIME_NO_LIB
#define BOOST_DATE_TIME_NO_LIB
#endif

#include <boost/rpc/service/async_tcp.hpp>
#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/detail/lightweight_test.hpp>

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

struct client
	: boost::rpc::service::async_asio_stream<client, header_variant, boost::asio::ip::tcp::socket, Serialize>
	, boost::enable_shared_from_this<client>
{
	client(asio::io_service& ios) : async_stream_base(ios) {}

	bool receive(const header_variant&, std::vector<char>&)
	{
	}
	void receive_error(boost::system::error_code)
	{

	}
};

struct server
	: boost::rpc::service::async_asio_stream<server, header_variant, boost::asio::ip::tcp::socket, Serialize>
	, boost::enable_shared_from_this<server>
{
	server(asio::io_service& ios) : async_stream_base(ios) {}

	bool receive(header_variant header, std::vector<char>& data)
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

	void receive_error(boost::system::error_code ec)
	{
		BOOST_ERROR(ec.message().c_str());
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
		g_client->async_send(header_b(value_header_b), std::vector<char>(data_header_b.begin(), data_header_b.end()), &on_send_header_b);
	}
}

void on_connect(boost::system::error_code ec)
{
	if(!ec)
	{
		test_results[test_client_connect] = true;
		g_client->async_send(header_a(value_header_a), std::vector<char>(), &on_send_header_a);
	}
}

int main()
{
	boost::asio::io_service ios;
	g_client.reset(new client(ios));
	g_server.reset(new server(ios));
	rpc::service::stream_connector<asio::ip::tcp> connector(ios);
	rpc::service::stream_acceptor<asio::ip::tcp> acceptor(ios, "127.0.0.1", "10001");
	acceptor.async_accept(g_server->next_layer(), &on_accept);
	connector.async_connect(g_client->next_layer(), "127.0.0.1", "10001", &on_connect);
	ios.run();
	BOOST_TEST(test_results[test_client_connect]);
	BOOST_TEST(test_results[test_server_accept]);
	BOOST_TEST(test_results[test_send_header_a]);
	BOOST_TEST(test_results[test_recv_header_a]);
	BOOST_TEST(test_results[test_send_header_b_with_data]);
	BOOST_TEST(test_results[test_recv_header_b_with_data]);
	g_client.reset();
	g_server.reset();
	return boost::report_errors();
}
