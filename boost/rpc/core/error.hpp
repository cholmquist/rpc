/*=============================================================================
    Copyright (c) 2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_ERROR_HPP
#define BOOST_RPC_ERROR_HPP

#include <boost/system/error_code.hpp>

namespace boost{
namespace rpc{

enum errors
{
	remote_exception = 1,
	serialization_error = 2,
};

namespace detail
{
	class error_category : public boost::system::error_category
	{
	public:

		virtual const char * name() const
		{
			return "rpc";
		}
		virtual std::string message( int ev ) const
		{
			if(ev == remote_exception)
				return "remote exception";
			else if(ev == serialization_error)
			{
				return "serialization_error";
			}
			return "rpc.error";
		}

	};
} // namespace detail

const boost::system::error_category& get_error_category()
{
	static detail::error_category instance;
	return instance;
}

inline boost::system::error_code make_error_code(errors e)
{
  return boost::system::error_code(
      static_cast<int>(e), boost::rpc::get_error_category());
}


} // namespace rpc

namespace system
{
	template<> struct is_error_code_enum<boost::rpc::errors>
	{
		static const bool value = true;
	};

} // namespace system

} // namespace boost

#endif
