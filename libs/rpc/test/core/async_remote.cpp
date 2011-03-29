/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include "signatures.hpp"
#include <vector>
#include <boost/rpc/core/async_remote.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/exception/current_exception_cast.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/function/function2.hpp>
#include <vector>
#include <string>

namespace rpc = boost::rpc;
using boost::system::error_code;

const char CHAR_RESULT = 5;

enum error_mode
{
	no_error,
	serialization_error,
	remote_exception_error,
	remote_exception_and_serialization_error,
};

struct service
{
	typedef std::vector<char> buffer_type;
	typedef std::string signature_id;

	typedef boost::function<void(const error_code&, buffer_type&)> response_callback;


	service(error_mode err)
		: m_handler_id_gen(0)
		, m_error_mode(err)
	{}

	void async_call(const signature_id& id, std::vector<char>& data)
	{
	}

/*	int async_send(async_call_header, std::vector<char>& data, response_callback c)
	{
		c(*this, error_code(), data);
		return 0;
	}*/

	void async_call(const signature_id& id, std::vector<char>& data, response_callback c)
	{
		std::vector<char> v;
		if(m_error_mode == no_error) // generate one char as respone
		{
			rpc::protocol::bitwise::writer w(rpc::protocol::bitwise(), v);
			w((char)CHAR_RESULT, rpc::tags::parameter());
			c(error_code(), v);
		}
		else if(m_error_mode == serialization_error) // response buffer not filled in
		{
			c(error_code(), v);
		}
		else if(m_error_mode == remote_exception_error) // generate an exception
		{
			rpc::protocol::bitwise::writer w(rpc::protocol::bitwise(), v);
			w(rpc_test::exception("test"), rpc::tags::parameter());
			c(rpc::remote_exception, v);
		}
		else if(m_error_mode == remote_exception_and_serialization_error) // response buffer not filled in
		{
			c(rpc::remote_exception, v);
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

void response_remote_exception_and_serialization_error(char x, error_code ec)
{
	BOOST_TEST(x == 0);
	BOOST_TEST(ec == rpc::remote_exception);
	BOOST_TEST(boost::current_exception_cast<rpc::protocol::bitwise_reader_error>() != 0);
}

int main()
{
	rpc::async_remote<rpc::protocol::bitwise> async_remote;

	{
		service s(no_error);
		async_remote(rpc_test::void_char, s, &response_no_error)(1);
	}
	{
		service s(serialization_error);
		async_remote(rpc_test::void_char, s, &response_serialization_error)(1);
	}
	{
		service s(remote_exception_error);
		async_remote(rpc_test::void_char, s, &response_remote_exception_error)(1);
	}
	{
		service s(remote_exception_and_serialization_error);
		async_remote(rpc_test::void_char, s, &response_remote_exception_and_serialization_error)(1);
	}
	return boost::report_errors();
}

