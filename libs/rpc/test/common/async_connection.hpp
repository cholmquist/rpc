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
      
  };

template<class StreamProtocol>
class stream_connector
{
	template<class Handler>
	struct async_iterator
	{
		typename StreamProtocol::socket& m_socket;
		typename StreamProtocol::resolver::iterator m_endpoint_itr;
		Handler m_handler;

		async_iterator(typename StreamProtocol::socket& sock, Handler h)
			: m_socket(sock)
			, m_endpoint_itr()
			, m_handler(h)
		{

		}

		// Handle connect
		void operator()(const boost::system::error_code& ec)
		{
			if(!ec)
			{
				m_handler(ec);
			}
			else
			{
				m_socket.close();
				if(++m_endpoint_itr != typename StreamProtocol::resolver::iterator())
				{
					m_socket.async_connect(*m_endpoint_itr, *this);
				}
				else
				{
					m_handler(ec);
				}
			}
		}

		// Handle resolve
		void operator()(const boost::system::error_code& ec, typename StreamProtocol::resolver::iterator endpoint_itr)
		{
			if(!ec)
			{
				m_endpoint_itr = endpoint_itr;
				m_socket.async_connect(*m_endpoint_itr, *this);
			}
			else
			{
				m_handler(ec);
			}
		}

	private:
		async_iterator& operator=(const async_iterator&);
	};

public:
	typename StreamProtocol::resolver::iterator m_endpoint_itr;
	boost::shared_ptr<typename StreamProtocol::resolver> m_resolver;

	stream_connector(boost::asio::io_service& ios)
		: m_endpoint_itr()
		, m_resolver(new typename StreamProtocol::resolver(ios))
	{
	}

	template<class Handler>
	void async_connect(typename StreamProtocol::socket& sock, const std::string& host_name, const std::string& service_name, Handler h)
	{
		m_resolver->async_resolve(typename StreamProtocol::resolver::query(host_name, service_name),
			async_iterator<Handler>(sock, h));
	}

};

template<class StreamProtocol>
class stream_acceptor
{
	template<class ConnectionPtr>
	struct accept_handler
	{
		accept_handler(ConnectionPtr ptr)	:
		m_ptr(ptr)
		{}

		void operator()(const boost::system::error_code& ec)
		{
			if(!ec)
				m_ptr->start();
			else
				m_ptr->accept_error(ec);
		}

	private:
		ConnectionPtr m_ptr;

	};

public:
	stream_acceptor(
		boost::asio::io_service& ios,
		const std::string& address,
		const std::string& service_name) :
		acceptor_(ios)
	{
		typename StreamProtocol::resolver resolver(ios);
		typename StreamProtocol::resolver::query query(address, service_name);
		typename StreamProtocol::endpoint endpoint = *resolver.resolve(query);
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(typename StreamProtocol::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();
	}

	template<class Handler>
	void async_accept(typename StreamProtocol::socket& sock, Handler h)
	{
		acceptor_.async_accept(sock, h);
	}

private:
	typename StreamProtocol::acceptor acceptor_;
};
  
}

#endif
