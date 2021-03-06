/*=============================================================================
    Copyright (c) 2012 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include "../common/signatures.hpp"
#include <boost/rpc/core/remote.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/exception/current_exception_cast.hpp>
#include <boost/detail/lightweight_test.hpp>
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

const char CHAR_RESULT = 'A';

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

void rpc_call(error_mode mode, const signature_id, std::vector<char>& in, std::vector<char>& out)
{
	if(mode == no_error) // generate one char as respone
	{
		bitwise::writer w(bitwise(), out);
		w((char)CHAR_RESULT, rpc::tags::parameter());
	}
	else if(mode == serialization_error) // response buffer not filled in
	{
	}
	else if(mode == remote_exception_error) // generate an exception
	{
		bitwise::writer w(bitwise(), out);
		w(rpc_test::exception("test"), rpc::tags::parameter());
	}
	else if(mode == remote_exception_and_serialization_error) // response buffer not filled in
	{
	}
	else if(mode == increment_no_error)
	{
		bitwise::writer w(bitwise(), out);
		w((int)2, rpc::tags::parameter());
	}
}

int main()
{
	rpc::remote<bitwise> remote;

	char result = 0;
	remote(rpc_test::void_char, no_error)((char)1, result);
	BOOST_TEST_EQ(result, CHAR_RESULT);
/*	async_remote(rpc_test::void_char, no_error, &response_no_error)(1);
	async_remote(rpc_test::void_char, serialization_error, &response_serialization_error)(1);
	async_remote(rpc_test::void_char, remote_exception_error, &response_remote_exception_error)(1);
	async_remote(rpc_test::void_char, remote_exception_and_serialization_error, &response_remote_exception_and_serialization_error)(1);*/
	BOOST_TEST(remote(rpc_test::increment, increment_no_error)(1) == 2);
	return boost::report_errors();
}
