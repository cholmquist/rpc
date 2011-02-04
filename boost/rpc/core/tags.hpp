/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_TAGS_HPP
#define BOOST_RPC_TAGS_HPP

namespace boost{ namespace rpc{ namespace tags
{
	struct default_ {};
	struct parameter : default_ {};
	struct result : public default_ {};
	struct exception : public default_ {};
	struct placeholder : public default_ {};
}}}

#endif
