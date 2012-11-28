/*=============================================================================
    Copyright (c) 2007-2011 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_TEST_CORE_SIGNATURES_HPP
#define BOOST_RPC_TEST_CORE_SIGNATURES_HPP

#include <boost/rpc/core/signature.hpp>
#include <boost/rpc/core/throws.hpp>
#include <boost/rpc/core/placeholder.hpp>
#include <boost/rpc/core/async_remote.hpp>
#include <boost/rpc/core/remote.hpp>
#include <boost/concept_check.hpp>
#include <exception>
#include <cstring>

namespace rpc_test
{
	struct exception : public std::exception
	{
		exception(){}
		exception(const char* w) /*: std::exception(w)*/ {}

		bool operator==(const exception& e) const
		{
			return std::strcmp(e.what(), this->what()) == 0;
		}
	};

	struct qquit_exception : std::exception
	{
	};

	template<class Signature, class Exception = boost::rpc::no_exception_handler>
	struct signature : boost::rpc::signature<std::string, Signature, Exception>
	{
		signature(std::string id)
			: boost::rpc::signature<std::string, Signature, Exception>(id) {}
	};

	const boost::rpc::placeholder<int> _context_id = {};
	
	
	static const signature<int(int), boost::rpc::throws<exception> > increment("increment");
	static const signature<void()> quit("quit");
	static const signature<void()> get_context_id("get_context_id"); 
	static const signature<std::string(const std::string&), boost::rpc::throws<exception> > reverse("reverse");
	static const signature<void(const std::string&, std::string&), boost::rpc::throws<exception> > reverse2("reverse2");
	
	namespace f1
	{
	    static const signature<void(char, char&), boost::rpc::throws<exception> > sig("f1");

	    static const char arg1 = 'Z';
	    static const char result = 'A';
	    static bool impl_called = false;
	    static bool async_handler_called = false;
	    static bool async_call_called = false;
	    static bool call_called = false;
	    
	    void impl(char in, char& out)
	    {
		impl_called = true;
		BOOST_TEST_EQ(in, arg1);
		out = result;
	    }
	    void async_handler(char out, const boost::system::error_code& ec)
	    {
		async_handler_called = true;
		BOOST_TEST_EQ(out, result);
		BOOST_TEST(!ec);
	    }

	    template<class Protocol, class Remote>
	    void async_call(Protocol p, Remote r)
	    {
		async_call_called = true;
		boost::rpc::async_remote<Protocol> async_remote(p);
		async_remote(sig, r, &async_handler)(arg1);
	    }

	    template<class Protocol, class Remote>
	    void call(Protocol p, Remote r)
	    {
		call_called = true;
		boost::rpc::remote<Protocol> remote(p);
		char out = 0;
		remote(sig, r)(arg1, out);
		BOOST_TEST_EQ(out, result);
	    }
	    
	    void verify_called()
	    {
		BOOST_TEST(impl_called);
		BOOST_TEST(async_handler_called);
		BOOST_TEST(async_call_called);
		//BOOST_TEST(call_called);
	    }
	}
	
	template<class Local, class FunctionMap>
	void register_all(Local local, FunctionMap& functions)
	{
	    functions.insert(local(f1::sig, &f1::impl));
	}
	
	void verify_all_called()
	{
	    f1::verify_called();
	}

}

#endif
