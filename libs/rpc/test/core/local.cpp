/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include "../common/signatures.hpp"
#include <boost/rpc/core/local.hpp>
#include <boost/rpc/protocol/bitwise.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace rpc = boost::rpc;
using boost::system::error_code;

char CHAR_INPUT = 'A';
char CHAR_OUTPUT = 'Z';

typedef std::vector<char> buffer_type;

struct input
{
	input(int i) : id(i) {}

	void clear() { buffer.clear(); }

	template<class T>
	void push_arg(T const& t)
	{
		rpc::protocol::bitwise_writer<buffer_type> w(buffer);
		w(t, rpc::tags::parameter());
	}

	std::vector<char> buffer;
	int id;
};

struct output
{
	buffer_type buffer;

	template<class T>
	T pop_arg()
	{
		T t = T();
		rpc::protocol::bitwise_reader<buffer_type> r(buffer);
		r(t, rpc::tags::parameter());
		return t;
	}

	void clear() { buffer.clear(); }
};

struct protocol
{
	typedef input input_type;
	typedef output output_type;

	struct reader : rpc::protocol::bitwise_reader<buffer_type>
	{
		reader(protocol p, input& i)
			: rpc::protocol::bitwise_reader<buffer_type>(i.buffer) 
		{}
	};

	struct writer : rpc::protocol::bitwise_writer<buffer_type>
	{
		writer(protocol p, output& o)
			: rpc::protocol::bitwise_writer<buffer_type>(o.buffer) 
		{}
	};
};

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

int main()
{
	rpc::local<protocol> l;
	input i(5);
	output o;
	{
		i.clear();
		o.clear();
		i.push_arg((char) CHAR_INPUT);
		l(rpc_test::void_char, &void_char).second(i, o);
		BOOST_TEST_EQ(o.pop_arg<char>(), CHAR_OUTPUT);
	}
	{
		i.clear();
		o.clear();
		i.push_arg((char) CHAR_INPUT);
		l(rpc_test::void_char, &void_char_throw).second(i, o);
		BOOST_TEST(o.pop_arg<rpc_test::exception>() == rpc_test::exception("test"));
	}
	return boost::report_errors();
}
