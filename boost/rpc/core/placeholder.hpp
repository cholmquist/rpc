#ifndef BOOST_RPC_CORE_PLACEHOLDER_HPP
#define BOOST_RPC_CORE_PLACEHOLDER_HPP

#include <boost/rpc/core/arg.hpp>
#include <boost/bind/arg.hpp>
#include <boost/bind/placeholders.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/less_equal.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/assert.hpp>
//#include <boost/mpl/sort.hpp>
#include <boost/mpl/if.hpp>

namespace boost{ namespace rpc {

template<class T, class Tag = tags::placeholder>
struct placeholder_arg
{
	typedef T type;
	typedef mpl::false_ is_read;
	typedef mpl::false_ is_write;

	placeholder_arg() {}

	template<class Protocol>
	type get(Protocol& p) const
	{
		return p(Tag());
	}

};

namespace detail
{
	template<class T, int I, class Tag = tags::placeholder>
	struct placeholder_impl
	{
		typedef mpl::int_<I - 1> position;
		typedef void is_rpc_placeholder;
		typedef placeholder_arg<T, Tag> type;
	};

	struct insert_placeholder
	{
		template<class Args, class T, class IsPlaceholder = void>
		struct apply
		{
			typedef typename mpl::push_back<Args, T>::type type;
		};

		template<class Args, class T>
		struct apply<Args, T, typename T::is_rpc_placeholder>
		{
			// If the line below asserts (compilation error)
			// one of your placeholders is out of range of the given argument list
			BOOST_MPL_ASSERT(( mpl::less_equal<typename T::position, mpl::size<Args> > ));
			typedef typename mpl::advance<typename mpl::begin<Args>::type, typename T::position>::type itr;
			typedef typename mpl::insert<Args, itr, typename T::type>::type type;
		};
	};

	// Forward declared in arg.hpp, where a more lightweight implementation of make_args exist without placeholder support
	template<typename Select, typename Args, typename Placeholders>
	struct make_args
	{
		typedef typename mpl::fold<
			mpl::joint_view<
				mpl::filter_view<
					mpl::transform_view<
						Args,
						make_arg<Select> >,
					traits::is_not_void_parameter>,
				Placeholders>,
				mpl::vector<>,
				insert_placeholder>::type type;
	};

	template<class P0, class P1>
	struct sort_placeholders_2 : mpl::if_c< ((P0::index) < (P1::index)),
		mpl::vector<P0, P1>, mpl::vector<P1, P0> >
	{
	};
}

template<class T>
struct placeholder
{
	typedef T type;

	template<class Arg>
	detail::placeholder_impl<type, is_placeholder<Arg>::value> operator=(Arg) const
	{
		return detail::placeholder_impl<type, is_placeholder<Arg>::value>();
	}
};

template<class P0>
mpl::vector1<P0> placeholders(P0)
{
	return mpl::vector1<P0>();
}

template<class P0, class P1>
typename detail::sort_placeholders_2<P0, P1>::type placeholders(P0, P1)
{
	return typename detail::sort_placeholders_2<P0, P1>::type();
}

}}

#endif
