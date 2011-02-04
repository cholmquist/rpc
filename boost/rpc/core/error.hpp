#ifndef BOOST_RPC_ERROR_HPP
#define BOOST_RPC_ERROR_HPP

#include <boost/system/error_code.hpp>

namespace boost{
namespace rpc{

enum errors
{
	remote_exception = 1,
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
			return "rpc.error";
		}

	};
} // namespace detail

const boost::system::error_category& get_error_category()
{
	static detail::error_category instance;
	return instance;
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
