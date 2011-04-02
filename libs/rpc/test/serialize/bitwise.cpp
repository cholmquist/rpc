/*=============================================================================
    Copyright (c) 2010-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#pragma warning(disable:4996)
#include "serialize_test.hpp"
#include <boost/rpc/protocol/bitwise.hpp>

int main()
{
	BOOST_STATIC_ASSERT((boost::rpc::traits::is_array<boost::array<char, 4> >::value ));
	BOOST_STATIC_ASSERT((boost::rpc::traits::is_array<std::vector<char> >::value == 0 ));

	rpc_test::serialize<boost::rpc::protocol::bitwise> test;
	test.run();
	// Boost.Serialization doesn't support fusion sequences at this moment, tests moved directly to bitwise
	test.roundtrip(boost::fusion::make_vector((char)1, (short)2, (double) 3));
	test.roundtrip(boost::variant<char, boost::fusion::vector<short, double> >(boost::fusion::make_vector((short) 65535, double(3))));

	return boost::report_errors();
}
