/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_CORE_ASYNC_REMOTE_HPP
#define BOOST_RPC_CORE_ASYNC_REMOTE_HPP

#include <boost/rpc/core/arg.hpp>
#include <boost/rpc/core/tags.hpp>
#include <boost/rpc/core/error.hpp>
#include <boost/mpl/single_view.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/functional/adapter/unfused_typed.hpp>
#include <boost/fusion/functional/invocation/invoke_procedure.hpp>
#include <boost/system/error_code.hpp>
//#include <boost/optional/optional.hpp>
//#include <boost/variant/variant.hpp>

namespace boost{ namespace rpc{

template<class SignatureID, class HandlerID>
struct async_call_header
{
	SignatureID signature_id;
	HandlerID handler_id;
	async_call_header(SignatureID sid, HandlerID hid)
		: signature_id(sid)
		, handler_id(hid)
	{}
};

namespace detail
{
	template<class Signature, class Placeholders = void>
	struct async_input_args
	{
		typedef typename fusion::result_of::as_vector<
			typename detail::make_args<traits::remote_of_,
			typename Signature::parameter_types, Placeholders>::type
		>::type arg_types;

		typedef typename fusion::result_of::as_vector<
			mpl::filter_view<arg_types, traits::is_write_>
		>::type type;
	};

/*
	template<class Error, class Exceptions>
	struct build_error_type
	{
		typedef typename boost::make_variant_over<
			typename mpl::push_back<Exceptions, Error>::type>::type error_variant;

		typedef boost::optional<error_variant> type;
	};

	template<class Error>
	struct build_error_type<Error, void>
	{
		typedef boost::optional<Error> type;
	};
*/

	template<class Signature, class Placeholders = void>
	struct async_output_args
	{
		typedef typename fusion::result_of::as_vector<
			typename detail::make_args<traits::remote_of_,
			typename Signature::parameter_types, Placeholders>::type
		>::type arg_types;

		typedef typename fusion::result_of::as_vector<
			typename detail::if_void<typename Signature::result_type,
				mpl::filter_view<arg_types, traits::is_read_>,
				mpl::joint_view<
					mpl::single_view<typename detail::make_result<traits::remote_of_, typename Signature::result_type>::type >,
					mpl::filter_view<arg_types, traits::is_read_>
				>
			>::type
		>::type type;
	};

	template<class Protocol, class Signature, class Remote>
	struct async_remote_procedure_impl
	{
		typedef void result_type;

		typedef typename Protocol::input_type input;
		typedef typename Protocol::writer writer;
		typedef typename async_call_header<typename Signature::id_type, typename Remote::handler_id> header_type;

		async_remote_procedure_impl(Protocol p, typename Signature::id_type id, Remote r)
			: p(p), id(id), r(r) {}

		template<class Args>
		void operator()(const Args& args)
		{
			input in; // This line may cause some trouble.. best would be to call p.make_input(), but not all buffer types are copyable (std::streambuf)
			fusion::for_each(args,
				functional::write_arg<writer>(writer(p, in)));
			header_type header(id, r.empty_handler_id());
			r.async_send(header, in);
		}

		Protocol p;
		Remote r;
		typename Signature::id_type id;
	};

	template<class Protocol>
	struct null_reader
	{
		null_reader(Protocol, const system::error_code& err)
			: err(err)
		{}

		template<class T, class Tag>
		void operator()(T&, Tag)
		{
		}

		template<class Tag>
		void operator()(system::error_code& ec, Tag)
		{
			ec = err;
		}

		system::error_code err;
	};

	template<class Protocol, class Signature, class Remote, class Handler>
	struct async_remote_impl
	{
		typedef typename async_output_args<Signature>::type output_arg_types;
		typedef typename Remote::handler_id handler_id;

		struct handler_base
		{
			handler_base(Protocol p, Handler h)
				: p(p), h(h) {}

			Protocol p;
			Handler h;
		};

		struct receive_handler : handler_base
		{
			typedef typename Protocol::output_type output;
			typedef typename Protocol::reader reader;
			typedef typename async_output_args<Signature>::type output_arg_types;

			receive_handler(Protocol p, Handler h) : handler_base(p, h) {}

			template<class Remote>
			void operator()(Remote& r, system::error_code const& err, std::vector<char>& input)
			{
				output_arg_types args;

				if(!err)
				{
					reader r(p, input);
					fusion::for_each(args,
						functional::read_arg<reader>(r));
					fusion::invoke_procedure(h,
						fusion::transform(args, functional::arg_type<reader>(r)));
				}
				else if(err.category() == get_error_category())
				{
					if(err.value() == remote_exception)
					{
					}
				}

			}
		};

		struct send_handler : handler_base
		{
			send_handler(Protocol p, Handler h, handler_id hid) : handler_base(p, h), hid(hid) {}

			void operator()(Remote& r, system::error_code const& err, std::vector<char>&)
			{
				if(!err)
				{
					r.async_receive(hid, receive_handler(p, h));
				}
				else
				{
					typedef null_reader<Protocol> reader;
					output_arg_types args;
					reader r(p, err);
					fusion::for_each(args,
						functional::read_arg<reader>(r));
					fusion::invoke_procedure(h,
						fusion::transform(args, functional::arg_type<reader>(r)));
				}
			}

			handler_id hid;

		};

		async_remote_impl(Protocol p, typename Signature::id_type id, Remote r, Handler h)
			: p(p), sigid(id), r(r), h(h) {}

		typedef void result_type;

		template<class Args>
		void operator()(const Args& args)
		{
			typedef typename Protocol::writer writer;
			typedef typename async_call_header<typename Signature::id_type, typename Remote::handler_id> header;

			Protocol::input_type input;
			fusion::for_each(args,
				functional::write_arg<writer>(writer(p, input)));
			header head(sigid, r.allocate_handler_id());
			r.async_send(head, input, 
				send_handler(p, h, head.handler_id));
		}

		Protocol p;
		Remote r;
		Handler h;
		typename Signature::id_type sigid;
	};
}

template<class Protocol, bool EnableExceptions = false>
struct async_remote
{
	Protocol p;
	async_remote(Protocol p = Protocol())
		: p(p) {}

	template<class Signature, class Remote>
	fusion::unfused_typed<detail::async_remote_procedure_impl<Protocol, Signature, Remote>,
		typename detail::async_input_args<Signature>::type>
	operator()(Signature s, Remote r) const
	{
		return fusion::unfused_typed<detail::async_remote_procedure_impl<Protocol, Signature, Remote>,
			typename detail::async_input_args<Signature>::type>(detail::async_remote_procedure_impl<
				Protocol, Signature, Remote>(p, s.id, r));
	}

	template<class Signature, class Remote, class Handler>
	fusion::unfused_typed<detail::async_remote_impl<Protocol, Signature, Remote, Handler>,
		typename detail::async_input_args<Signature>::type>
	operator()(Signature s, Remote r, Handler h) const
	{
		return fusion::unfused_typed<detail::async_remote_impl<Protocol, Signature, Remote, Handler>,
			typename detail::async_input_args<Signature>::type>(detail::async_remote_impl<Protocol, Signature, Remote, Handler>(p, s.id, r, h));
	}

};

}}

#endif
