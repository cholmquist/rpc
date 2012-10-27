#ifndef BOOST_RPC_ASYNC_TCP_HPP
#define BOOST_RPC_ASYNC_TCP_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <vector>

namespace boost{
namespace rpc{

  template<class Derived>
  class async_tcp
  {
    
  public:
      explicit async_tcp(boost::asio::io_service& ios)
	: m_socket(ios)
      {}
      
      template<class Buffers>
      void do_async_send(const Buffers& buffers)
      {
		  boost::asio::async_write(m_socket,
			  buffers, 
			  boost::bind(&Derived::async_send_completed, static_cast<Derived>(this)->shared_from_this(), _1, _2));
	
      }

      template<class Buffers>
      void do_async_receive(const Buffers& buffers)
      {
	  m_socket.async_read_some(
	    buffers,
	    boost::bind(&Derived::async_receive_completed, static_cast<Derived>(this)->shared_from_this(), _1, _2));
      }
      
      boost::asio::ip::tcp::socket& socket() { return m_socket;}

  private:
	boost::asio::ip::tcp::socket m_socket;
  };

  
}}

#endif
