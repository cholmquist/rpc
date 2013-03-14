#ifndef BOOST_RPC_ASYNC_TCP_HPP
#define BOOST_RPC_ASYNC_TCP_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>

namespace boost
{
namespace rpc
{
namespace detail
{

template<class ConnectionPtr>
struct connect_handler {
    connect_handler ( ConnectionPtr ptr )	:
        m_ptr ( ptr )
    {}

    void operator() ( const boost::system::error_code& ec ) {
        m_ptr->connected ( ec );
    }

private:
    ConnectionPtr m_ptr;

};

class tcp_acceptor
{
public:
    typedef boost::asio::ip::tcp tcp;

    tcp_acceptor (
        boost::asio::io_service& ios,
        const std::string& address,
        const std::string& service_name ) :
        acceptor_ ( ios ) {
        tcp::resolver resolver ( ios );
        tcp::resolver::query query ( address, service_name );
        tcp::endpoint endpoint = *resolver.resolve ( query );
        acceptor_.open ( endpoint.protocol() );
        acceptor_.set_option ( tcp::acceptor::reuse_address ( true ) );
        acceptor_.bind ( endpoint );
        acceptor_.listen();
    }

    template<class ConnectionPtr>
    void async_accept ( ConnectionPtr connection ) {
        acceptor_.async_accept ( connection->socket(), connect_handler<ConnectionPtr> ( connection ) );
    }

private:
    tcp::acceptor acceptor_;
};

class tcp_connector
{
    typedef boost::asio::ip::tcp tcp;
    template<class Handler>
    struct async_iterator {
        tcp::socket& m_socket;
        tcp::resolver::iterator m_endpoint_itr;
        Handler m_handler;

        async_iterator ( tcp::socket& sock, Handler h )
            : m_socket ( sock )
            , m_endpoint_itr()
            , m_handler ( h ) {

        }

        // Handle connect
        void operator() ( const boost::system::error_code& ec ) {
            if ( !ec ) {
                m_handler ( ec );
            } else {
                m_socket.close();
                if ( ++m_endpoint_itr != tcp::resolver::iterator() ) {
                    m_socket.async_connect ( *m_endpoint_itr, *this );
                } else {
                    m_handler ( ec );
                }
            }
        }

        // Handle resolve
        void operator() ( const boost::system::error_code& ec, tcp::resolver::iterator endpoint_itr ) {
            if ( !ec ) {
                m_endpoint_itr = endpoint_itr;
                m_socket.async_connect ( *m_endpoint_itr, *this );
            } else {
                m_handler ( ec );
            }
        }

    private:
        async_iterator& operator= ( const async_iterator& );
    };

public:
    tcp::endpoint m_endpoint;

    tcp_connector ( boost::asio::io_service& ios, const std::string& host_name, const std::string& service_name )
        : m_endpoint() {
        tcp::resolver resolver ( ios );
        tcp::resolver::query query ( host_name, service_name );
        m_endpoint = *resolver.resolve ( query );
    }

    template<class ConnectionPtr>
    void async_connect ( ConnectionPtr connection ) {
        connection->socket().async_connect ( m_endpoint, connect_handler<ConnectionPtr> ( connection ) );
    }

};


} //detail

template<class Derived>
class async_tcp
{

public:
    typedef async_tcp<Derived> async_tcp_t;
    typedef detail::tcp_acceptor acceptor;
    typedef detail::tcp_connector connector;

    explicit async_tcp ( boost::asio::io_service& ios )
        : m_socket ( ios )
    {}

    template<class Buffers>
    void async_write ( const Buffers& buffers )
    {
        boost::asio::async_write ( m_socket,
                                   buffers,
                                   boost::bind ( &Derived::completed_async_write, static_cast<Derived*> ( this )->shared_from_this(), _1, _2 ) );

    }

    template<class Buffers>
    void async_read_some ( const Buffers& buffers )
    {
        m_socket.async_read_some (
            buffers,
            boost::bind ( &Derived::completed_async_read_some, static_cast<Derived*> ( this )->shared_from_this(), _1, _2 ) );
    }

    template<class Handler>
    void post_ts(const Handler& handler)
    {
	m_socket.get_io_service().post(handler);
    }
    
    boost::asio::ip::tcp::socket& socket() {
        return m_socket;
    }

private:
    boost::asio::ip::tcp::socket m_socket;
};


} //rpc
} //detail

#endif
