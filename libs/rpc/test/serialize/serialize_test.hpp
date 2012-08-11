/*=============================================================================
    Copyright (c) 2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_TEST_SERIALIZE_HPP
#define BOOST_RPC_TEST_SERIALIZE_HPP

#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/variant/variant.hpp>
#include <boost/array.hpp>
#include <boost/rpc/core/tags.hpp>
#include <string>
#include <vector>

namespace rpc_test
{
	struct udt
	{
		udt() : value(0) {}
		udt(int v, std::string s) : value(v), str(s) {}
		int value;
		std::string str;

		bool operator==(const udt& rhs) const
		{
			return value == rhs.value && str == rhs.str;
		}

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & value;
			ar & str;
		}

	};

template<class Serialize>
struct serialize
{
	void run()
	{
		namespace fusion = boost::fusion;
		using boost::variant;
		using boost::array;

		roundtrip((char)1);
		roundtrip((short)1);
		roundtrip((int)1);
		roundtrip((double)1);
		roundtrip(std::string("test"));
		roundtrip(variant<char, std::string>(std::string("test2")));
		array<int, 4> a = {1, 100, 42, 10001};
		roundtrip(a);
//		roundtrip(udt(5, "test3"));
	}

	template<class T>
	void roundtrip(const T& x)
	{
		T y = T();
		BOOST_ASSERT(!(x == y));
		std::vector<char> data;
		Serialize p;
		
		{
			typename Serialize::writer w(p, data);
			w(x, boost::rpc::tags::parameter());
		}
		{
			typename Serialize::reader r(p, data);
			r(y, boost::rpc::tags::parameter());
		}
		BOOST_TEST(x == y);
	}

};

}

#endif
