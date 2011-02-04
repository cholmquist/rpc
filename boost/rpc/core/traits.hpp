/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/


#ifndef BOOST_RPC_TRAITS_HPP_INCLUDED
#define BOOST_RPC_TRAITS_HPP_INCLUDED

#include <boost/mpl/bool.hpp>

namespace boost{ namespace rpc{

	template<typename T>
	struct default_arg;

	template<typename T>
	struct default_result;

	namespace detail
	{

		// arg_of
		template<typename T, typename Void = void>
		struct arg_of
		{
			typedef default_arg<T> type;
		};

		template<typename T>
		struct arg_of<T, typename T::is_rpc_arg_>
		{
			typedef T type;
		};

		// result_arg_of
		template<typename T, typename Void = void>
		struct result_arg_of
		{
			typedef default_result<T> type;
		};

		template<typename T>
		struct result_arg_of<T, typename T::is_rpc_result>
		{
			typedef T type;
		};

		// if_void
		template<typename Void, typename T1, typename T2>
		struct if_void
		{
			typedef T2 type;
		};

		template<typename T1, typename T2>
		struct if_void<void, T1, T2>
		{
			typedef T1 type;
		};

	}

namespace traits
{
	// is_mutable
	template<typename T>
	struct is_mutable : mpl::false_
	{};

	template<typename T>
	struct is_mutable<T const&> : mpl::false_
	{};

	template<typename T>
	struct is_mutable<T&> : mpl::true_
	{};

	// value_of
	template<typename T>
	struct value_of
	{
		typedef T type;
	};

	template<typename T>
	struct value_of<T const&>
	{
		typedef T type;
	};

	template<typename T>
	struct value_of<T&>
	{
		typedef T type;
	};


	// arg_of
	template<typename T>
	struct arg_of : rpc::detail::arg_of<T>
	{
	};

	// return_of
	template<typename T>
	struct result_arg_of : rpc::detail::result_arg_of<T>
	{
	};

	struct local_of_
	{
		template<typename P>
		struct apply
		{
			typedef typename P::local type;
		};
	};

	struct remote_of_
	{
		template<typename P>
		struct apply
		{
			typedef typename P::remote type;
		};
	};

	struct is_read_
	{
		template<class T>
		struct apply : T::is_read
		{
		};
	};

	// is_write
	struct is_write_
	{
		template<class T>
		struct apply : T::is_write
		{
		};
	};

	struct is_not_void_parameter
	{
		template<typename P>
		struct apply
		{
			typedef typename rpc::detail::if_void<
				typename P::type, mpl::false_, mpl::true_>::type type;
		};
	};

}

}}

#endif
