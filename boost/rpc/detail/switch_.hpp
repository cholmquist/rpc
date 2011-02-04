/*=============================================================================
    Copyright (c) 2010-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_DETAIL_SWITCH
#ifndef BOOST_PP_IS_ITERATING

#ifndef BOOST_RPC_SWITCH_LIMITS
#define BOOST_RPC_SWITCH_LIMITS 4
#endif

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/mpl/int.hpp>

namespace boost{ namespace rpc{ namespace detail{

template<int Max>
struct switch_t;

template<int Max, class F>
bool switch_(int n, F f)
{
	return switch_t<Max>::call(n, f);
}

#define BOOST_PP_FILENAME_1 <boost/rpc/detail/switch_.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_RPC_SWITCH_LIMITS)
#include BOOST_PP_ITERATE()

}}}

#define BOOST_RPC_DETAIL_SWITCH

#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

#define M(z, n, data) case n:	f(mpl::int_<n>());	return true;

template<>
struct switch_t<N>
{
	template<class F> static bool call(int n, F f)
	{
		switch(n)
		{
			BOOST_PP_REPEAT(N, M, ~)
		}
		return false;
	}
};

#undef N
#undef M

#endif // defined(BOOST_PP_IS_ITERATING)
#endif
