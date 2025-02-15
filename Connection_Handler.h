#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include <boost/enable_shared_from_this.hpp>
#include "Client_Manager.h"

class Connection_Handler : public boost::enable_shared_from_this<Connection_Handler> {
private: 
    boost::asio::ip::tcp::socket connection_socket;
    std::string client_id; // unique identifier for the client (string "IP:port")
    Client_Manager& client_manager; 
    unsigned int unique_client_number; // client number assigned by Client_Manager
    std::string message = "Hello from Server! Your unique ID is #"; // message to clients
    boost::asio::streambuf read_buffer; // dynamic buffer to read from the socket
public:
    typedef boost::shared_ptr<Connection_Handler> pointer;


public:
    Connection_Handler(boost::asio::io_context& io_context, Client_Manager& manager);
    ~Connection_Handler();
        
    // create shared pointer
    static pointer create(boost::asio::io_context& io_context, Client_Manager& manager);

    // access socket
    boost::asio::ip::tcp::socket& socket();   

    // send greeting and then read client message
    void start();

    // compose client IP and port into client ID (string "ID:port")
    void retrieve_client_id(); 

private:
    void do_read();

    void handle_read(const boost::system::error_code& err, std::size_t bytes_transferred);
};

#endif // CONNECTION_HANDLER_H