/*=============================================================================
    Copyright (c) 2012 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_COMMANDS_HPP_INCLUDED
#define BOOST_RPC_COMMANDS_HPP_INCLUDED

#include <boost/variant/variant.hpp>

namespace boost{ namespace rpc {

template<class FunctionID, class CallID = char>
struct commands
{
	typedef CallID call_id_type;
	typedef FunctionID function_id_type;
	struct call
	{
		CallID call_id;
		FunctionID function_id;
		call()
			: call_id()
			, function_id()
		{}

		template<class Archive>
		void serialize(Archive& ar, unsigned int)
		{
			ar & call_id;
			ar & function_id;
		}
	};

	struct result
	{
		CallID call_id;
		result()
			: call_id()
		{}

		result(char call_id)
			: call_id(call_id)
		{}

		template<class Archive>
		void serialize(Archive& ar, unsigned int)
		{
			ar & call_id;
		}
	};

	struct result_exception
	{
		CallID call_id;
		result_exception()
			: call_id()
		{}

		result_exception(char call_id)
			: call_id(call_id)
		{}

		template<class Archive>
		void serialize(Archive& ar, unsigned int)
		{
			ar & call_id;
		}
	};
	
	typedef boost::variant<call, result, result_exception> variant_type;

};

}}

#endif //BOOST_RPC_COMMANDS_HPP_INCLUDED
