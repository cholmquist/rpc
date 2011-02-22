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
#include <boost/bind.hpp>
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
	: boost::rpc::service::async_asio_stream<client, header_variant, boost::asio::ip::tcp::socket, rpc::protocol::bitwise>
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
	: boost::rpc::service::async_asio_stream<server, header_variant, boost::asio::ip::tcp::socket, rpc::protocol::bitwise>
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


void on_accept(boost::system::error_code ec, server_ptr server)
{
	if(!ec)
	{
		test_results[test_server_accept] = true;
		server->start();
	}
}

void on_send_header_b(client_ptr client, std::vector<char>&, const boost::system::error_code& ec)
{
	if(!ec)
	{
		test_results[test_send_header_b_with_data] = true;
	}
}

void on_send_header_a(client_ptr client, std::vector<char>&, const boost::system::error_code& ec)
{
	if(!ec)
	{
		test_results[test_send_header_a] = true;
		client->async_send(header_b(value_header_b), std::vector<char>(data_header_b.begin(), data_header_b.end()), &on_send_header_b);
	}
}

void on_connect(boost::system::error_code ec, client_ptr client)
{
	if(!ec)
	{
		test_results[test_client_connect] = true;
		client->async_send(header_a(value_header_a), std::vector<char>(), &on_send_header_a);
	}
}

int main()
{
	boost::asio::io_service ios;
	client_ptr client(new client(ios));
	server_ptr server(new server(ios));
	rpc::service::stream_connector<asio::ip::tcp> connector(ios);
	rpc::service::stream_acceptor<asio::ip::tcp> acceptor(ios, "127.0.0.1", "10000");
	acceptor.async_accept(server->next_layer(), boost::bind(&on_accept, _1, server));
	connector.async_connect(client->next_layer(), "127.0.0.1", "10000", boost::bind(&on_connect, _1, client));
	ios.run();
	BOOST_TEST(test_results[test_client_connect]);
	BOOST_TEST(test_results[test_server_accept]);
	BOOST_TEST(test_results[test_send_header_a]);
	BOOST_TEST(test_results[test_recv_header_a]);
	BOOST_TEST(test_results[test_send_header_b_with_data]);
	BOOST_TEST(test_results[test_recv_header_b_with_data]);
	return boost::report_errors();
}
