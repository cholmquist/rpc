#ifndef BOOST_RPC_REMOTE_HPP_INCLUDED
#define BOOST_RPC_REMOTE_HPP_INCLUDED

#include <boost/rpc/core/traits.hpp>
#include <boost/rpc/core/arg.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/filter_if.hpp>
#include <boost/fusion/functional/adapter/unfused_typed.hpp>

namespace boost{ namespace rpc{

template<class Protocol, class Signature, class Function>
struct remote_impl
{
	typedef typename fusion::result_of::as_vector<
		typename detail::make_args<traits::remote_of,
		typename Signature::parameter_types>::type
	>::type args_type;

	typedef typename detail::make_result<
		traits::remote_of, typename Signature::result_type>::type result_arg;

	typedef typename Protocol::reader reader;
	typedef typename Protocol::writer writer;
	typedef typename Protocol::input input;
	typedef typename Protocol::output output;

	struct function
	{
		function(Protocol p, typename Signature::id_type id, Function f)
			: p(p)
			, id(id)
			, f(f)
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
			input in;
			fusion::for_each(
				fusion::filter_if<traits::is_write_>(args),
				functional::write_arg<writer>(writer(p, in)));
			output out;
			exception_id eid = f(id, in, out);
			reader r(p, out);
			if(eid)
			{
				Signature::exception_handler::throw_(eid, functional::read_exception<reader>(r));
			}
			else
			{
				result_arg result;
				result.read(r);
				fusion::for_each(
					fusion::filter_if<traits::is_read_>(args),
					functional::read_arg<Protocol>(p));
				return result.get(r);
			}
		}

		Protocol p;
		Function f;
		typename Signature::id_type id;
	};
};

template<class Protocol>
struct remote
{
	Protocol p;
	remote(Protocol p = Protocol())
		: p(p)
	{}

	template<class S, class F>
	fusion::unfused_typed<typename remote_impl<Protocol, S, F>::function,
		typename remote_impl<Protocol, S, F>::args_type>
	operator()(S s, F f)
	{
		return fusion::unfused_typed<remote_impl<Protocol, S, F>::function,
			typename remote_impl<Protocol, S, F>::args_type>
			(typename remote_impl<Protocol, S, F>::function(p, s.id, f));
	}
};

}}

#endif
