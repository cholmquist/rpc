/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_SIGNATURE_HPP_INCLUDED
#define BOOST_RPC_SIGNATURE_HPP_INCLUDED

#include <boost/rpc/core/exception.hpp>
#include <boost/function_types/components.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/cstdint.hpp>

namespace boost{ namespace rpc
{
	struct no_exception_handler
	{
		typedef void exception_types;

		template<class Function, class Handler>
		static exception_id catch_(Function f, Handler)
		{
			f();
			return exception_id();
		}

		template<class Protocol>
		static void throw_(exception_id, Protocol&)
		{
			throw rpc::unknown_exception();
		}
	};

	template<class Id, class Signature, class ExceptionHandler = no_exception_handler>
	struct signature
	{
	public:
		typedef Id id_type;
		typedef Signature type;
		typedef typename function_types::components<
			Signature, remove_reference<mpl::_> >::types component_types;
		typedef typename mpl::at_c<component_types, 0>::type result_type;
		typedef typename mpl::pop_front<component_types>::type parameter_types;
		typedef ExceptionHandler exception_handler;

		signature(id_type id)
			: id(id)
		{}

		id_type id;
	};

	template<class Signature, class ExceptionHandler = no_exception_handler>
	struct signature_c : signature<boost::uint8_t, Signature, ExceptionHandler>
	{
		signature_c(boost::uint8_t id)
			: signature<boost::uint8_t, Signature, ExceptionHandler>(id)
		{}

	};

}}

#endif
