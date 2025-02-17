#include "Server.h"
#include <iostream>

// constructor: initializes the server, binds to the given IP and port, and starts accepting connections
Server::Server(boost::asio::io_context& io_context, const std::string& ip_address, const int port) 
    : server_acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(ip_address), port)),
      client_manager(io_context) {
    Logger::get_instance().log_debug("Server initialized on " + ip_address + " : " + std::to_string(port));
    
    start_connections(); // begin accepting client connections
}

// destructor: logs server shutdown
Server::~Server() {
    Logger::get_instance().log_debug("Server destructor called");
}


// starts asynchronously accepting incoming client connections
void Server::start_connections() {
    // get reference to io_context from the acceptor's executor
    boost::asio::io_context& io_context = static_cast<boost::asio::io_context&>(server_acceptor.get_executor().context());

    // create a new connection handler instance 
    auto connection = Connection_Handler::create(io_context, client_manager);

    // asynchronously wait for a new client to connect
    server_acceptor.async_accept(
        connection->socket(),
        [this, connection](const boost::system::error_code& error) {
            on_client_connected(connection, error); // handle the connection attempt
        });

    // print "waiting for clients" only once (atomic flag ensures this)
    if (is_waiting.exchange(false)) { // atomic check and set
        std::cout << "\nWaiting for clients to connect on [" << ip_address << " : " << port << "] ...\n";
        Logger::get_instance().log_info("(Server::start_connections()) : Waiting for clients to connect on [" + ip_address + " : " + std::to_string(port) + "] ...");
    }        
}

// handles the acceptance of a new client connection
void Server::on_client_connected(Connection_Handler::pointer connection, const boost::system::error_code& err) {
    if (!err) {
        connection->start(); // start communication with the client
    }
    start_connections(); // continue accepting new client connections
}