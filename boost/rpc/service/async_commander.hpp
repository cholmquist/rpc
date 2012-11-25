/*=============================================================================
    Copyright (c) 2012 Christian Holmquist

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_RPC_ASYNC_COMMANDER_HPP_INCLUDED
#define BOOST_RPC_ASYNC_COMMANDER_HPP_INCLUDED

#include <boost/rpc/core/error.hpp>
#include <boost/rpc/core/exception.hpp>
#include <boost/function/function2.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <map>

namespace boost {
namespace rpc {

template<class Derived, class Commands, class FunctionMap>
class async_commander
{
public:
    typedef async_commander<Derived, Commands, FunctionMap> async_commander_t;
    typedef FunctionMap function_map_type;
    typedef typename Commands::function_id_type function_id_type;
    typedef typename Commands::call_id_type call_id_type;
    typedef typename Commands::variant_type variant_type;
    typedef boost::function<void(std::vector<char>& data, const boost::system::error_code&)> result_handler;
    typedef std::vector<char> buffer_type;


    explicit async_commander(function_map_type& function_map)
        : m_function_map(function_map)
        , m_call_id_enum(0)
    {
    }
    
    ~async_commander()
    {
    }

    void receive(const variant_type& command, std::vector<char>& buffer)
    {
        return boost::apply_visitor(command_visitor(static_cast<Derived*>(this), buffer), command);
    }
    
    void async_call(const function_id_type& id, std::vector<char>& data, const result_handler& handler)
    {
        typename Commands::call call;
        if(handler)
        {
            call.call_id = m_call_id_enum++;
            if(m_result_handlers.insert(typename result_handler_map::value_type(call.call_id, handler)).second == false)
            {
                buffer_type empty;
                handler(empty, rpc::internal_error);
                return;
            }
        }
        else
        {
            call.call_id = 0;
        }
        call.function_id = id;
        (static_cast<Derived*>(this))->async_send(call, data);
    }

protected:
    
    void command(const typename Commands::call& c, buffer_type& input_buffer)
    {
        std::vector<char> output_buffer;
	typename function_map_type::const_iterator itr = m_function_map.find(c.function_id);
	if(itr != m_function_map.end())
	{
	    try
	    {
	      itr->second(input_buffer, output_buffer);
	      (static_cast<Derived*>(this))->async_send(typename Commands::result(c.call_id), output_buffer);
	    }
	    catch(const abort_exception&)
	    {
	      output_buffer.clear(); // reuse the buffer
	      (static_cast<Derived*>(this))->async_send(typename Commands::result_exception(c.call_id), output_buffer);
	      throw;
	    }
	}
    }

    void command(const typename Commands::result& r, buffer_type& buffer)
    {
	    this->invoke_result_handler(r.call_id, buffer, boost::system::error_code());
    }

    void command(const typename Commands::result_exception& r, buffer_type& buffer)
    {
	this->invoke_result_handler(r.call_id, buffer, rpc::remote_exception);
    }

    void invoke_result_handler(call_id_type id, buffer_type& buffer, const boost::system::error_code& ec)
    {
	    typename result_handler_map::iterator handler_itr = m_result_handlers.find(id);
	    if(handler_itr != m_result_handlers.end())
	    {
		    result_handler handler;
		    handler_itr->second.swap(handler);
		    m_result_handlers.erase(handler_itr);
		    handler(buffer, ec);
	    }
	    else
	    {
		    // Unknown call_id
	    }
    }

private:

    struct command_visitor : boost::static_visitor<void>
    {
	command_visitor(Derived* t, buffer_type& buffer)
	    : m_this(t)
	    , m_buffer(buffer)
	{}
	template<class Command>
	void operator()(const Command& c) const
	{
	    return m_this->command(c, m_buffer);
	}
	Derived* m_this;
	buffer_type& m_buffer;
    };

    friend struct command_visitor;

    typedef std::map<call_id_type, result_handler> result_handler_map;

    function_map_type& m_function_map;
    result_handler_map m_result_handlers;
    char m_call_id_enum;


};

}
}

#endif //BOOST_RPC_ASYNC_COMMANDER_HPP_INCLUDED
