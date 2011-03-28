/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_ARG_HPP_INCLUDED
#define BOOST_RPC_ARG_HPP_INCLUDED

#include <boost/rpc/core/traits.hpp>
#include <boost/rpc/core/tags.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/not.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost{ namespace rpc {

	struct arg
	{
		typedef void is_rpc_arg_;
	};

	template<typename T, typename Tag = tags::parameter>
	struct local_arg
	{
		typedef Tag protocol_tag;
		typedef T type;
		typedef traits::is_mutable<T> is_write;
		typedef mpl::not_<is_write> is_read;

		local_arg() : value() {}

		template<typename Protocol>
		void read(Protocol& p) const
		{
			p(value, protocol_tag());
		}

		template<typename Protocol>
		void write(Protocol& p) const
		{
			p(value, protocol_tag());
		}

		mutable typename traits::value_of<T>::type value;

		template<class Protocol>
		type get(Protocol&) const { return value; }
	};

	template<typename T>
	struct remote_arg
	{
		typedef tags::parameter protocol_tag;
		typedef T type;
		typedef mpl::false_ is_read;
		typedef mpl::true_ is_write;

		T value;
		remote_arg(T t) : value(t) {}

		template<class Protocol>
		void write(Protocol& p) const { p(value, protocol_tag()); }

		type to_type() const { return value; }
	};

	template<typename T>
	struct remote_arg<const T&>
	{
		typedef tags::parameter protocol_tag;
		typedef const T& type;
		typedef mpl::false_ is_read;
		typedef mpl::true_ is_write;

		mutable T value;

		template<class X>
		remote_arg(const X& x) : value(x) {}

		template<class Protocol>
		void write(Protocol& p) const { p(value, protocol_tag()); }

		type to_type() const { return value; }
	};

	template<typename T>
	struct remote_arg<T&>
	{
		typedef tags::parameter protocol_tag;
		typedef T& type;
		typedef mpl::true_ is_read;
		typedef mpl::false_ is_write;

		mutable T value;

		template<class Protocol>
		void read(Protocol& p) const
		{
			p(value, protocol_tag());
		}

		type to_type() const { return value; }

		template<class Protocol>
		type get(Protocol&) const
		{
			return value;
		}
	};

	template<typename T>
	struct default_result
	{
		struct local
		{
			typedef T type;
			typedef tags::result tag;
			
			local(T t) : value(t) {}

			template<class Protocol>
			void write(Protocol &p) const
			{
				p(value, tag());
			}

			template<class Protocol>
			type get(Protocol&) const
			{
				return value;
			}

			T value;
		};
		struct remote
		{
			typedef tags::result protocol_tag;
			typedef T type;
			typedef tags::result tag;
			typedef mpl::true_ is_read;
			typedef mpl::false_ is_write;

			mutable T value;

			remote() : value() {}

			template<class Protocol>
			void read(Protocol& p) const
			{
				p(value, tag());
			}

			template<class Protocol>
			type get(Protocol&) const
			{
				return value;
			}

			type to_type() const { return value; }
		};
	};
	template<>
	struct default_result<void>
	{
		typedef void local;
		struct remote
		{
			typedef void type;
			remote() {}

			template<class Protocol>
			void read(Protocol&)
			{}

			template<class Protocol>
			type get(Protocol&) const
			{}

		};
	};

	template<typename T>
	struct default_arg
	{
		typedef local_arg<T> local;
		typedef remote_arg<T> remote;
	};

	namespace functional
	{

		template<typename Protocol>
		struct read_arg
		{
			typedef void result_type;
			Protocol& p_;
			read_arg(Protocol& p) : p_(p) {}

			template<typename Arg>
			void operator()(Arg& arg) const
			{
				arg.read(p_);
			}

		};

		template<typename Protocol>
		struct write_arg
		{
			typedef void result_type;
			Protocol& p_;
			write_arg(Protocol& p) : p_(p) {}

			template<typename Arg>
			void operator()(Arg& arg) const
			{
				arg.write(p_);
			}
		};

		template<typename Protocol, class Tag = tags::exception>
		struct write_exception
		{
			typedef void result_type;
			Protocol& p_;
			write_exception(Protocol& p) : p_(p) {}

			template<typename Exception>
			void operator()(const Exception& e) const
			{
				p_(e, Tag());
			}
		};

		template<typename Protocol, class Tag = tags::exception>
		struct read_exception
		{
			typedef void result_type;
			Protocol& p_;
			read_exception(Protocol& p) : p_(p) {}

			template<typename Exception>
			void operator()(Exception& e) const
			{
				p_(e, Tag());
			}
		};

		template<class Protocol>
		struct arg_type
		{
			Protocol& p_;
			arg_type(Protocol& p) : p_(p) {}
			template<typename T>
			struct result;

			template<typename P>
			struct result<arg_type(P)>
			{
				typedef typename boost::remove_reference<P>::type parameter;
				typedef typename parameter::type type;
			};

			template<typename T>
			typename result<arg_type(T)>::type operator()(T &t) const
			{
				return t.get(p_);
			}
		};

	}

	namespace detail
	{

		template<typename Select>
		struct make_arg
		{
			template<typename T>
			struct apply
			{
				typedef typename Select::template apply<
					typename traits::arg_of<T>::type>::type type;
			};
		};

		template<typename Select, typename T>
		struct make_result
		{
			typedef typename Select::template apply<
					typename traits::result_arg_of<T>::type>::type type;
		};

		template<typename Select, typename Args, typename Placeholders>
		struct make_args;

		template<typename Select, typename Args>
		struct make_args<Select, Args, void>
		{
			typedef typename mpl::filter_view<
				mpl::transform_view<
				Args,
				make_arg<Select> >,
				traits::is_not_void_parameter> type;
		};

		template<typename Select, typename Args>
		struct args_view : mpl::filter_view<mpl::transform_view<Args, make_arg<Select> >, traits::is_not_void_parameter>
		{};
	}

}}

#endif
