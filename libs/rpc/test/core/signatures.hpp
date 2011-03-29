/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_TEST_CORE_SIGNATURES_HPP
#define BOOST_RPC_TEST_CORE_SIGNATURES_HPP

#include <boost/rpc/core/signature.hpp>
#include <boost/rpc/core/throws.hpp>
#include <cstring>

namespace rpc_test
{
	struct exception : public std::exception
	{
		exception(){}
		exception(const char* w) : std::exception(w) {}

		bool operator==(const exception& e) const
		{
			return std::strcmp(e.what(), this->what()) == 0;
		}
	};

	boost::rpc::signature<std::string, void(char, char&), boost::rpc::throws<exception> > void_char("test");
}

#endif
