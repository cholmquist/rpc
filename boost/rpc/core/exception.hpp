#ifndef BOOST_RPC_EXCEPTION_HPP
#define BOOST_RPC_EXCEPTION_HPP

#include <boost/array.hpp>
#include <exception>

namespace boost{ namespace rpc{

struct exception_id
{
	typedef char value_type;

	boost::array<char, 1> data;

	exception_id()
	{
		data[0] = -1;
	}

	explicit exception_id(value_type index)
	{
		data[0] = index;
	}

	value_type value() const { return data[0]; }

	operator bool() const { return data[0] != -1; }

};

struct unknown_exception : public std::exception
{
	virtual const char* what() const throw()
	{
		return "Unknown rpc exception";
	};
};

}}

#endif
