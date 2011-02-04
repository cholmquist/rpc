/*=============================================================================
    Copyright (c) 2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_CORE_ASYNC_LOCAL_HPP
#define BOOST_RPC_CORE_ASYNC_LOCAL_HPP

#include <boost/rpc/core/arg.hpp>
#include <boost/rpc/core/tags.hpp>
#include <boost/mpl/single_view.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/functional/adapter/unfused_typed.hpp>
#include <boost/fusion/functional/invocation/invoke_procedure.hpp>



namespace boost{ namespace rpc{

namespace detail
{

}

template<class Protocol>
struct async_local
{
	Protocol p;
	async_local(Protocol p = Protocol())
		: p(p) {}

};

}}

#endif
