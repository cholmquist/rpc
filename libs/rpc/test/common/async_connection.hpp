#ifndef BOOST_RPC_TEST_ASYNC_CONNECTION_H
#define BOOST_RPC_TEST_ASYNC_CONNECTION_H

#include <boost/rpc/service/async_stream_connection.hpp>
#include <boost/rpc/service/async_tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/bind.hpp>

namespace rpc_test
{
  template<class Header, class Serialize>
  class connection
    : public boost::rpc::service::async_stream_connection<connection<Header, Serialize>, Header, Serialize>
    , public boost::rpc::async_tcp<connection<Header, Serialize> >
    , public boost::enable_shared_from_this<connection<Header, Serialize> >
  {
    
  public:
      connection(boost::asio::io_service& ios, std::size_t recvsize = 64)
	: boost::rpc::service::async_stream_connection<connection, Header, Serialize>(recvsize)
	, boost::rpc::async_tcp<connection<Header, Serialize> >(ios)
      {}
      
      virtual void receive_error(boost::system::error_code ec) = 0;

      virtual bool receive(const Header&, std::vector<char>&) = 0;
      
      virtual void connected(boost::system::error_code ec) = 0;
      
  };

}

#endif
