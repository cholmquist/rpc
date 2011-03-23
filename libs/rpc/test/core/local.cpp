//#include "signatures.hpp"
#include <boost/mpl/vector.hpp>
#include <boost/mpl/max_element.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/static_assert.hpp>
#include <boost/variant/variant.hpp>
#include <boost/rpc/detail/switch_.hpp>
#include <boost/rpc/core/throws.hpp>

	namespace mpl = boost::mpl;

struct ta
{
	static const int static_length = 1;
};

struct tb
{
	static const int static_length = 2;
};


template<class HeaderVariant>
struct max_header_length
{
	struct cmp
	{
		template<class Left, class Right>
		struct apply : mpl::if_c< ((Left::static_length) < (Right::static_length)), mpl::true_, mpl::false_>
		{
		};
	};

	static const int value = mpl::deref<
		typename mpl::max_element<typename HeaderVariant::types, cmp>::type
	>::type::static_length;
};

template<class HeaderVariant>
struct min_header_length
{
	struct cmp
	{
		template<class Left, class Right>
		struct apply : mpl::if_c< ((Left::static_length) > (Right::static_length)), mpl::true_, mpl::false_>
		{
		};
	};

	static const int value = mpl::deref<
		typename mpl::max_element<typename HeaderVariant::types, cmp>::type
	>::type::static_length;
};

struct handle_int
{
	template<int N>
	void operator()(boost::mpl::int_<N>)
	{

	}
};

void test()
{
	BOOST_STATIC_ASSERT((max_header_length<boost::variant<ta, tb> >::value == 2));
	BOOST_STATIC_ASSERT((min_header_length<boost::variant<ta, tb> >::value == 1));
	typedef mpl::vector<ta, tb> types;
	boost::rpc::detail::switch_<4>(0, handle_int());
//	typedef mpl::max_element<types,	less_static_length>::type itr;
//	BOOST_STATIC_ASSERT((mpl::deref<itr>::type::static_length == 2));
};

#if 0
#include "boost/rpc/local.hpp"
#include "boost/rpc/signature.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace boost;

struct impl0
{
	void operator()()
	{
	}
};

struct p0
{
	typedef void result_type;
	typedef int context;
	p0(int& ctx, char)
	{

	}
};

struct impl1
{
	typedef void result_type;
	void operator()(int i)
	{
		BOOST_TEST(i == 3);
	}
};

struct p1
{
	typedef void result_type;
	typedef int context;
	p1(int& ctx, char)
	{

	}

	void read(int& i, rpc::tags::parameter)
	{
		i = 3;
	}
};

typedef impl0 impl2;
typedef p0 p2;

struct impl4
{
	typedef void result_type;

	bool& invoked_;

	impl4(bool& invoked) : invoked_(invoked) {}

	void operator()(mpl::int_<1>)
	{
		invoked_ = true;
	}
};

struct p4
{
	typedef void result_type;
	typedef int context;

	p4(int& ctx, char)
	{
	}

	void read(mpl::int_<1>&, rpc::tags::parameter)
	{

	}
};

static void test_impl_by_ref()
{
//	local<rpc_test::sig_0, p0, impl0> a0(i0);
//	a0(ctx);
}

#ifdef BOOST_RPC_TEST_NO_MAIN
extern void test_local()
#else
int main()
#endif
{
	int ctx = 0;

	{
		impl0 i0;
		rpc::local<rpc::signature<char, void()>, p0, impl0> a0(i0);
		a0(ctx);
	}

	{
		impl1 i1;
		rpc::local<rpc::signature<char, void(int)>, p1, impl1> a0(i1);
		a0(ctx);
	}

	/*	{
		impl2 i2;
		rpc::local<rpc_test::sig_2, p2, impl2> a0(i2);
		a0(ctx);
	}

	{	// sig_4
		bool invoked_impl4 = false;
		impl4 i(invoked_impl4);
		rpc::local<rpc_test::sig_4, p4, impl4> a0(i);
		a0(ctx);
		BOOST_TEST(invoked_impl4);
	}*/
	
#ifndef BOOST_RPC_TEST_NO_MAIN
	return boost::report_errors();
#endif
}
#endif
