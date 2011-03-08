#ifndef RPC_THROWS_HPP_INCLUDED
#ifndef BOOST_PP_IS_ITERATING

#ifndef BOOST_RPC_THROWS_LIMITS
#define BOOST_RPC_THROWS_LIMITS 4
#endif

#include <boost/rpc/core/exception.hpp>
#include <boost/throw_exception.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>

namespace boost { namespace rpc {

namespace detail
{
       struct na;
}

template<BOOST_PP_ENUM_BINARY_PARAMS(BOOST_RPC_THROWS_LIMITS, class E, = detail::na BOOST_PP_INTERCEPT)>
struct throws;

#define BOOST_PP_FILENAME_1 <boost/rpc/core/throws.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_RPC_THROWS_LIMITS)
#include BOOST_PP_ITERATE()

}}

#define RPC_THROWS_HPP_INCLUDED

#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

#define M_catch(z, n, data) catch(const E##n &e) { h(e); return exception_id(n);  }
#define M_throw(z, n, data) if(n == en) { E##n e; h(e); boost::throw_exception(e); }

#if N > 0

	template<BOOST_PP_ENUM_PARAMS(N, class E)>
	struct throws
#	if N < BOOST_RPC_THROWS_LIMITS
	<BOOST_PP_ENUM_PARAMS(N, E)>
#	endif
	{
		template<class Function, class Handler>
		static exception_id catch_(Function f, Handler h)
		{
			try
			{
				f();
			} BOOST_PP_REPEAT(N, M_catch, ~);
			return exception_id();
		}

		template<class Handler>
		BOOST_ATTRIBUTE_NORETURN static void throw_(exception_id eid, Handler h)
		{
			exception_id::value_type en = eid.value();
			BOOST_PP_REPEAT(N, M_throw, ~);
			boost::throw_exception(rpc::unknown_exception());
		}

};

#else

template<>
struct throws<>
{
	template<class Function, class Handler>
	static void catch_(Function f, Handler)
	{
		f();
	}
	template<class Protocol>
	BOOST_ATTRIBUTE_NORETURN static void throw_(exception_id, Protocol&)
	{
		boost::throw_exception(rpc::unknown_exception());
	}
};

#endif

#undef M_catch
#undef M_throw

#endif // defined(BOOST_PP_IS_ITERATING)
#endif
