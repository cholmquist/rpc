/*=============================================================================
    Copyright (c) 2010-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_DETAIL_INIT_VARIANT
#define BOOST_RPC_DETAIL_INIT_VARIANT

#include <boost/rpc/detail/switch_.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>

namespace boost{ namespace rpc { namespace detail {

template<class Variant>
struct set_variant_type
{
	set_variant_type(Variant& v) : m_variant(v) {}

	template<class N>
	void operator()(N) const
	{
		typedef typename mpl::at<typename Variant::types, N>::type type;
		m_variant = type();
	}

	Variant& m_variant;
};

template<class Variant>
bool init_variant(int which, Variant& v)
{
	return switch_<mpl::size<typename Variant::types>::value>
		(which, set_variant_type<Variant>(v));
}

template<class Variant, class InitFunction>
bool init_variant(int which, Variant& v, InitFunction f)
{
	bool result = switch_<mpl::size<typename Variant::types>::value>
		(which, set_variant_type<Variant>(v));
	if(result)
	{
		boost::apply_visitor(f, v);
	}
	return result;
}


}}}


#endif
