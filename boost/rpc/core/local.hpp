/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_CORE_LOCAL_HPP_INCLUDED
#define BOOST_RPC_CORE_LOCAL_HPP_INCLUDED

#include <boost/rpc/core/traits.hpp>
#include <boost/rpc/core/arg.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/filter_if.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/functional/invocation/invoke.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/system/error_code.hpp>
#include <utility>

namespace boost{ namespace rpc
{
	namespace detail
	{

		template<class Function, class Args, class Writer, class Result>
		struct local_invoke
		{
			Function& f;
			const Args& args;
			Writer& w;
			local_invoke(Function& f, const Args& args, Writer& w)
				: f(f), args(args), w(w)
			{}

			void operator()()
			{
				Result(fusion::invoke(f, 
					fusion::transform(args, functional::arg_type<Writer>(w)))).write(w);
			}
		};

		template<class Function, class Args, class Writer>
		struct local_invoke<Function, Args, Writer, void>
		{
			Function& f;
			const Args& args;
			Writer& w;
			local_invoke(Function& f, const Args& args, Writer& w)
				: f(f), args(args), w(w)
			{}

			void operator()()
			{
				fusion::invoke(f,
					fusion::transform(args, functional::arg_type<Writer>(w)));
			}
		};

	}
	template<class Protocol, class Signature, class Function, class Placeholders = void>
	struct local_impl
	{
		Protocol p;
		Function f;
		local_impl(Protocol p, Function f)
			: p(p)
			, f(f)
		{}

		typedef typename fusion::result_of::as_vector
		<
			typename detail::make_args<traits::local_of_,
			typename Signature::parameter_types, Placeholders>::type
		>::type args_type;

		typedef typename detail::make_result<
				traits::local_of_, typename Signature::result_type>::type internal_result_type;

		typedef typename Protocol::reader reader;
		typedef typename Protocol::writer writer;
		typedef typename Protocol::input_type input;
		typedef typename Protocol::output_type output;

		typedef exception_id result_type;

		exception_id operator()(input& in, output& out)
		{
			args_type args;

			{
				reader r(p, in);
				fusion::for_each(
					fusion::filter_if<traits::is_read_>(args),
					functional::read_arg<reader>(r));
			}
			{
				writer w(p, out);
				exception_id e = Signature::exception_handler::catch_(detail::local_invoke
						<Function, args_type, writer, internal_result_type>(f, args, w),
					functional::write_exception<writer>(w));
				if(e)
				{
					//int xxx = 0;
				}
				else
				{
					fusion::for_each(
						fusion::filter_if<traits::is_write_>(args),
						functional::write_arg<writer>(w));
				}
				return e;
			}
		}
	};

	template<class Protocol>
	struct local
	{
		Protocol p;
		local(Protocol p = Protocol())
			: p(p)
		{}

		template<class Signature, class Function>
		std::pair<typename Signature::id_type, local_impl<Protocol, Signature, Function> >
		operator()(Signature s, Function f) const
		{
			return std::pair<typename Signature::id_type, local_impl<Protocol, Signature, Function> >
				(s.id, local_impl<Protocol, Signature, Function>(p, f));
		}

		template<class Signature, class Function, class Placeholders>
		std::pair<typename Signature::id_type, local_impl<Protocol, Signature, Function, Placeholders> >
		operator()(Signature s, Function f, Placeholders) const
		{
			return std::pair<typename Signature::id_type, local_impl<Protocol, Signature, Function, Placeholders> >
				(s.id, local_impl<Protocol, Signature, Function, Placeholders>(p, f));
		}
	};

}}

#endif
