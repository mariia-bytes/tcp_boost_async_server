#ifndef CONNECTION_HANDLER_CPP
#define CONNECTION_HANDLER_CPP

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

unsigned short port = 55000; // default port

using namespace boost::asio;
using ip::tcp;

class Connection_Handler : public boost::enable_shared_from_this<Connection_Handler> {
private: 
    tcp::socket connection_socket;
    std::string message = "Hello from Server!\n";
    enum { max_length = 1024 };
    char data[max_length];

public:
    typedef boost::shared_ptr<Connection_Handler> pointer;

    Connection_Handler(boost::asio::io_context& io_context);

    // create a shared pointer
    static Connection_Handler::pointer create(boost::asio::io_context& io_context) {
        return pointer(new Connection_Handler(io_context));
    }

    // access socket
    tcp::socket& socket();

    void start();
    
    void handle_read(const boost::system::error_code& err, size_t bytes_transferred);
    

    void handle_write(const boost::system::error_code& err, size_t bytes_transferred);
};

#endif // CONNECTION_HANDLER_CPP

