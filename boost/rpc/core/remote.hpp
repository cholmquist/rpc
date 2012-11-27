/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_REMOTE_HPP_INCLUDED
#define BOOST_RPC_REMOTE_HPP_INCLUDED

#include <boost/rpc/core/traits.hpp>
#include <boost/rpc/core/arg.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/rpc/core/error.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/filter_if.hpp>
#include <boost/fusion/functional/adapter/unfused_typed.hpp>

namespace boost{ namespace rpc{

namespace detail
{
  
	template<class Protocol, class Signature, class Remote>
	struct remote_impl
	{
		typedef typename fusion::result_of::as_vector<
			typename detail::make_args<traits::remote_of_,
			typename Signature::parameter_types, void>::type
		>::type args_type;

		typedef typename detail::make_result<
			traits::remote_of_, typename Signature::result_type>::type result_arg;

		struct function
		{
			function(Protocol p, typename Signature::id_type id, Remote r)
				: p(p)
				, id(id)
				, r(r)
			{}

			template<typename>
			struct result;

			template <class Self>
			struct result< Self(args_type&) >
			{
				typedef typename result_arg::type type;
			};

			typename result_arg::type operator()(args_type& args)
			{
				typename Protocol::input_type in;
				{
					typename Protocol::writer w(this->p, in);
					fusion::for_each(
						fusion::filter_if<traits::is_write_>(args),
						functional::write_arg<typename Protocol::writer>(w));
				}
				typename Protocol::output_type out;
				rpc_call(r, id, in, out);
				typename Protocol::reader r(p, out);
/*				if(eid)
				{
					Signature::exception_handler::throw_(eid, functional::read_exception<reader>(r));
				}
				else*/
				{
					result_arg result;
					result.read(r);
					fusion::for_each(
						fusion::filter_if<traits::is_read_>(args),
						functional::read_arg<typename Protocol::reader>(r));
					return result.get(r);
				}
			}

			Protocol p;
			Remote r;
			typename Signature::id_type id;
		};
	};
	
}

template<class Protocol>
struct remote
{
	Protocol p;
	remote(Protocol p = Protocol())
		: p(p)
	{}
	
	template<class Signature, class Remote>
	struct result
	{
	  typedef detail::remote_impl<Protocol, Signature, Remote> impl_type;
	  typedef fusion::unfused_typed<typename impl_type::function, typename impl_type::args_type> type;
	};

	
	template<class Signature, class Remote>
	typename result<Signature, Remote>::type
	operator()(Signature s, Remote r) const
	{
		typedef result<Signature, Remote> result_t;
		return typename result_t::type(typename result_t::impl_type::function(p, s.id, r));

	}
};

}}

#endif
