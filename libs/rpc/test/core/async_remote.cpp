#include <vector>
#include <boost/rpc/core/signature.hpp>
#include <boost/rpc/core/async_remote.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/rpc/core/throws.hpp>
#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/exception/current_exception_cast.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/function/function3.hpp>
#include <boost/variant/variant.hpp>
#include <vector>
#include <string>

namespace rpc = boost::rpc;
using boost::system::error_code;

const int CHAR_RESULT = 5;


struct some_exception : public std::exception
{
	some_exception(){}
	some_exception(const char* w) : std::exception(w) {}
};

rpc::signature<std::string, void(char, char&), rpc::throws<some_exception> > void_char("test");

enum error_mode
{
	no_error,
	serialization_error,
	remote_exception_error,
};

struct service
{
	typedef std::vector<char> buffer_type;
	typedef int handler_id;
	typedef rpc::async_call_header<std::string, handler_id> async_call_header;
	typedef std::string signature_id;

	typedef boost::function<void(service&, const error_code&, buffer_type&)> response_callback;


	service(error_mode err)
		: m_handler_id_gen(0)
		, m_error_mode(err)
	{}

	handler_id allocate_handler_id()
	{
		return ++m_handler_id_gen;
	}

	handler_id empty_handler_id()
	{
		return 0;
	}

	void async_send(async_call_header, std::vector<char>& data)
	{
	}

	int async_send(async_call_header, std::vector<char>& data, response_callback c)
	{
		c(*this, error_code(), data);
		return 0;
	}

	void async_receive(handler_id hid, response_callback c)
	{
		std::vector<char> v;
		if(m_error_mode == no_error) // generate one char as respone
		{
			rpc::protocol::bitwise::writer w(rpc::protocol::bitwise(), v);
			w((char)CHAR_RESULT, rpc::tags::parameter());
			c(*this, error_code(), v);
		}
		else if(m_error_mode == serialization_error) // response buffer not filled in, will trigger exception bitwise::reader
		{
			c(*this, error_code(), v);
		}
		else if(m_error_mode == remote_exception_error)
		{
			rpc::protocol::bitwise::writer w(rpc::protocol::bitwise(), v);
			w(some_exception("test"), rpc::tags::parameter());
			c(*this, rpc::remote_exception, v);
		}
	}

private:
	int m_handler_id_gen;
	error_mode m_error_mode;
};

void response_no_error(char x, error_code ec)
{
	BOOST_TEST(x == CHAR_RESULT);
	BOOST_TEST(!ec);
}

void response_serialization_error(char x, error_code ec)
{
	BOOST_TEST(x == 0);
	BOOST_TEST(ec == rpc::serialization_error);
	BOOST_TEST(boost::current_exception_cast<rpc::protocol::bitwise_reader_error>() != 0);
}

void response_remote_exception_error(char x, error_code ec)
{
	BOOST_TEST(x == 0);
	BOOST_TEST(ec == rpc::remote_exception);
	BOOST_TEST(boost::current_exception_cast<std::exception>() != 0);
}

int main()
{
	rpc::async_remote<rpc::protocol::bitwise> async_remote;

	{
		service s(no_error);
		async_remote(void_char, s, &response_no_error)(1);
	}
	{
		service s(serialization_error);
		async_remote(void_char, s, &response_serialization_error)(1);
	}
	{
		service s(remote_exception_error);
		async_remote(void_char, s, &response_remote_exception_error)(1);
	}
	return boost::report_errors();
}

