#ifndef BOOST_RPC_SERVICE_FUNCTION_HPP
#define BOOST_RPC_SERVICE_FUNCTION_HPP
#include <boost/intrusive/list_hook.hpp>

namespace boost{ namespace rpc{

namespace detail
{
	template<class Result, class A0, class Base, class Derived>
	Result invoke_handler_base(Base* base, A0 a0)
	{
	//	return (*(static_cast<Derived*>(base)))(a0);
	}
}

template<class Result, class Arg>
class intrusive_function_base : public boost::intrusive::list_base_hook<>
{
public:
	typedef Result result_type;

	result_type invoke(Arg a)
	{
		return pf_derived_(this, a);
	}

protected:
	template<class Derived>
	intrusive_function_base(Derived*)
	{
		pf_derived_ = &detail::invoke_handler_base<result_type, Arg, intrusive_function_base, Derived>;
	}

private:
	result_type (*pf_derived_)(intrusive_function_base*, Arg);

};

#pragma warning(disable:4355)

template<class Derived, class Arg>
class intrusive_function : public intrusive_function_base<void, Arg>
{
public:
	intrusive_function() :
	intrusive_function_base(static_cast<Derived*>(this))
	{

	}
};
}}

#endif
