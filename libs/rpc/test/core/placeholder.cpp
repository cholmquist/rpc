#include <boost/rpc/core/placeholder.hpp>
#include <boost/rpc/core/arg.hpp>
#include <boost/fusion/container/list.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/less_equal.hpp>

using namespace boost;

template<class T>
struct p
{
	typedef rpc::placeholder_arg<T> type;
};

template<class T>
struct l
{
	typedef typename rpc::default_arg<T>::local type;
};

typedef rpc::detail::make_args<
rpc::traits::local_of, mpl::vector<int, float>, mpl::vector<rpc::detail::placeholder_impl<char, 1> > >::type args_1;
BOOST_MPL_ASSERT(( mpl::equal<args_1, mpl::vector<p<char>::type, l<int>::type, l<float>::type>>));

/*
template<class T>
struct p
{
	typedef rpc::placeholder_arg<T> type;

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
		BOOST_MPL_ASSERT(( mpl::less_equal<typename T::position, mpl::size<Args> > ));
		typedef typename mpl::advance<typename mpl::begin<Args>::type, typename T::position>::type itr;
		typedef typename mpl::insert<Args, itr, typename T::local>::type type;
	};
};

template<class Args, class Placeholders>
struct insert_placeholders
{
	typedef typename mpl::fold<mpl::joint_view<Args, Placeholders>, mpl::vector<>, insert_placeholder>::type type;
//	typedef Args type;
};

typedef mpl::vector<int, float> args_type;
typedef mpl::vector<rpc::detail::placeholder_impl<char, 1> > placeholder_types;
typedef mpl::vector<char, int, float> expected_types;

BOOST_MPL_ASSERT(( mpl::equal<insert_placeholders<args_type, placeholder_types>::type, mpl::vector<p<char>::type, int, float>>));
*/



/*
template<class Sequence, class Pos, class T>
struct insert_at
{
	typedef typename mpl::advance<typename mpl::begin<Sequence>::type, Pos>::type itr;
	typedef typename mpl::insert<Sequence, itr, T>::type type;
};

typedef insert_at<mpl::vector<>, mpl::int_<0>, int>::type vector1_int;
BOOST_MPL_ASSERT(( mpl::equal<vector1_int, mpl::vector<int>>));

typedef insert_at<fusion::list<>, mpl::int_<0>, int>::type vector1_int;
*/
