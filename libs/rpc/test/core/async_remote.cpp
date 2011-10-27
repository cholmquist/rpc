/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include "../common/signatures.hpp"
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


const char CHAR_RESULT = 5;

enum error_mode
{
	no_error,
	serialization_error,
	remote_exception_error,
	remote_exception_and_serialization_error,
	increment_no_error,
};


typedef std::vector<char> buffer_type;
typedef std::string signature_id;
typedef boost::function<void(buffer_type&, const error_code&)> response;

void rpc_async_call(error_mode mode, const signature_id, std::vector<char>& data, response c)
{
	std::vector<char> v;
	if(mode == no_error) // generate one char as respone
	{
		bitwise::writer w(bitwise(), v);
		w((char)CHAR_RESULT, rpc::tags::parameter());
		c(v, error_code());
	}
	else if(mode == serialization_error) // response buffer not filled in
	{
		c(v, error_code());
	}
	else if(mode == remote_exception_error) // generate an exception
	{
		bitwise::writer w(bitwise(), v);
		w(rpc_test::exception("test"), rpc::tags::parameter());
		c(v, rpc::remote_exception);
	}
	else if(mode == remote_exception_and_serialization_error) // response buffer not filled in
	{
		c(v, rpc::remote_exception);
	}
	else if(mode == increment_no_error)
	{
		bitwise::writer w(bitwise(), v);
		w((int)1, rpc::tags::parameter());
		c(v, error_code());
	}
}

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

void response_increment(int i, error_code ec)
{
	BOOST_TEST(i == 1);
}

int main()
{
	rpc::async_remote<bitwise> async_remote;

	async_remote(rpc_test::void_char, no_error, &response_no_error)(1);
	async_remote(rpc_test::void_char, serialization_error, &response_serialization_error)(1);
	async_remote(rpc_test::void_char, remote_exception_error, &response_remote_exception_error)(1);
	async_remote(rpc_test::void_char, remote_exception_and_serialization_error, &response_remote_exception_and_serialization_error)(1);
	async_remote(rpc_test::increment, increment_no_error, &response_increment)(1);
	return boost::report_errors();
}

