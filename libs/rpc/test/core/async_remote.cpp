#include <vector>
#include <boost/rpc/core/signature.hpp>
#include <boost/rpc/core/async_remote.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/function/function3.hpp>
#include <boost/variant/variant.hpp>
#include <vector>
#include <string>

namespace rpc = boost::rpc;
using boost::system::error_code;

const int CHAR_RESULT = 5;


rpc::signature<std::string, void(char, char&)> void_char("test");

struct service
{
	typedef std::vector<char> buffer_type;
	typedef int handler_id;
	typedef rpc::async_call_header<std::string, handler_id> async_call_header;
	typedef std::string signature_id;

	typedef boost::function<void(service&, const error_code&, buffer_type&)> response_callback;


	service(bool raise_exception)
		: m_handler_id_gen(0)
		, m_raise_exception(raise_exception)
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
		v.push_back(CHAR_RESULT);
		c(*this, error_code(), v);
	}

private:
	int m_handler_id_gen;
	bool m_raise_exception;
};

struct char_handler
{
	void operator()(char x, error_code ec)
	{
		BOOST_TEST(x == CHAR_RESULT);
	}
};

int main()
{
	rpc::async_remote<rpc::protocol::bitwise> async_remote;
	service s_no_except(false);
	service s_except(false);
	char x = 0;
	async_remote(void_char, s_no_except)(1);
	async_remote(void_char, s_no_except, char_handler())(1);
	async_remote(void_char, s_except)(1);
	return boost::report_errors();
}

