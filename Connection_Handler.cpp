#include "Connection_Handler.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

Connection_Handler::Connection_Handler(boost::asio::io_context& io_context) 
        : connection_socket(io_context) {}


// access socket
tcp::socket& Connection_Handler::socket() {
    return connection_socket;
}


void Connection_Handler::start() {
    // capture shared ownership of the handler
    auto self = shared_from_this();

    // send a message to the client asynchronously
    connection_socket.async_write_some(
                boost::asio::buffer(message),
                [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                    if (!err) {
                        std::cout << "\nMessage sent to client: " << self->message << "\n";
                    } else {
                        std::cerr << "Write error: " << err.message() << "\n";
                    }
                });
        
    connection_socket.async_read_some(
            boost::asio::buffer(data, max_length),
            boost::bind(&Connection_Handler::handle_read,
                        shared_from_this(),
                        boost::placeholders::_1,
                        boost::placeholders::_2));
        
    boost::asio::async_write(
            connection_socket,
            boost::asio::buffer(message),
            boost::bind(&Connection_Handler::handle_write,
                        shared_from_this(),
                        boost::placeholders::_1,
                        boost::placeholders::_2));
        
    }
    
void Connection_Handler::handle_read(const boost::system::error_code& err, size_t bytes_transferred) {
    if (!err) {
        // Null-terminate the received data to handle it as a string
        // data[bytes_transferred] = '\0';
        if (bytes_transferred < max_length) {
            data[bytes_transferred] = '\0';
        }
        
        std::cout << "Received message: " << data << "\n";

        // Clear the buffer to remove any leftover data
        std::fill(std::begin(data), std::end(data), 0);
        
        // Continue reading more data from the client
        connection_socket.async_read_some(boost::asio::buffer(data, max_length),
                                              boost::bind(&Connection_Handler::handle_read,
                                                          shared_from_this(),
                                                          boost::placeholders::_1,
                                                          boost::placeholders::_2));
    } else {
        std::cerr << "Read error: " << err.message() << "\n";
        connection_socket.close();
    }
}
    

void Connection_Handler::handle_write(const boost::system::error_code& err, size_t bytes_transferred) {
    if (!err) {
        std::cout << "\nServer sent: " << message << "\n";
    } else {
        std::cerr << "\nWrite error: " << err.message() << "\n";
        connection_socket.close();
    }
}