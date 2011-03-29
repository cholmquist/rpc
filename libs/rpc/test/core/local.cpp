/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include "signatures.hpp"
#include <boost/rpc/core/local.hpp>
#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace rpc = boost::rpc;
namespace protocol = boost::rpc::protocol;
using boost::system::error_code;

char CHAR_INPUT = 6;
char CHAR_OUTPUT = 20;

void void_char(char in, char& out)
{
	BOOST_TEST(in == CHAR_INPUT);
	out = CHAR_OUTPUT;
}

void void_char_throw(char in, char& out)
{
	out = CHAR_OUTPUT;
	boost::throw_exception(rpc_test::exception("test"));
}

template<class Buffer, class T>
void push_arg(Buffer &b, const T& t)
{
	protocol::bitwise::writer w(protocol::bitwise(), b);
	w(t, rpc::tags::parameter());
}

template<class T, class Buffer>
T pop_arg(Buffer& b)
{
	T t = T();
	protocol::bitwise::reader r(protocol::bitwise(), b);
	r(t, rpc::tags::parameter());
	return t;
}


int main()
{
	rpc::local<rpc::protocol::bitwise> l;
	std::vector<char> input, output;
	{
		input.clear(); output.clear();
		push_arg(input, (char) CHAR_INPUT);
		l(rpc_test::void_char, &void_char).second(input, output);
		BOOST_TEST(pop_arg<char>(output) == CHAR_OUTPUT);
	}
	{
		input.clear(); output.clear();
		push_arg(input, (char) CHAR_INPUT);
		l(rpc_test::void_char, &void_char_throw).second(input, output);
		BOOST_TEST(pop_arg<rpc_test::exception>(output) == rpc_test::exception("test"));
	}
	return boost::report_errors();
}
