#include "Connection_Handler.h"
#include <iostream>


// constructor: initializes the connection handler with a given io_context and client manager
Connection_Handler::Connection_Handler(boost::asio::io_context& io_context, Client_Manager& manager) 
    : connection_socket(io_context), client_manager(manager) {
    Logger::get_instance().log_debug("Connection Handler initialized");
}


// destructor: Cleans up resources, ensuring the socket is closed properly
Connection_Handler::~Connection_Handler() {
    try {   
        Logger::get_instance().log_debug("Connection Handler destructor called");   
        if (connection_socket.is_open()) {
            boost::system::error_code ec;
            connection_socket.close(ec);
            if (ec) {
                Logger::get_instance().log_error("(~Connection_Handler) : Socket close error: " + ec.message());
            }
        }

    } catch (const std::exception& e) {
        Logger::get_instance().log_error(e);
    }
}

        
// factory method: creates and returns a shared pointer to a new Connection_Handler instance
Connection_Handler::pointer Connection_Handler::create(boost::asio::io_context& io_context, Client_Manager& manager) {
    return pointer(new Connection_Handler(io_context, manager));
}


// getter for the socket: provides access to the connection socket
boost::asio::ip::tcp::socket& Connection_Handler::socket() {
    Logger::get_instance().log_info("(Connection_Handler::socket()) : Access to socket granted");
    return connection_socket;
}   


// start the connection handling: send an initial message to the client and begins reading
void Connection_Handler::start() {
    auto self = shared_from_this(); // capture shared ownership
  
    retrieve_client_id(); // get the client ID (IP:port)

    // retrieve or register the client in the Client_Manager
    auto client_number_opt = client_manager.get_client_number(client_id);
    if (!client_number_opt) { // if the client ID is not yet registered, add it
        client_manager.add_client(client_id, self);
        client_number_opt = client_manager.get_client_number(client_id);
    }

    unique_client_number = client_number_opt.value();

    message += std::to_string(unique_client_number) + "\n";

    // send an initial message asynchronously
    connection_socket.async_write_some(
        boost::asio::buffer(message),
        [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
            if (!err) {
                std::cout << "\nMessage sent to client #" << self->unique_client_number << ": " << self->message;
                Logger::get_instance().log_info("(Connection_Handler::start()) : Message sent to client #" + std::to_string(self->unique_client_number) + ": " + self->message);
            } else {
                std::cerr << "Write error: " << err.message() << "\n";
                Logger::get_instance().log_error("(Connection_Handler::start()) : Write error: " + err.message());
            }
        });
    do_read(); // start reading from the client
}

// retrieved the client ID by extracting IP and port from the socket
void Connection_Handler::retrieve_client_id() {
    try { 
        if (connection_socket.is_open()) {
            auto endpoint = connection_socket.remote_endpoint();
            client_id = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
            std::cout << "\nClient connected: " << client_id << "\n";
            Logger::get_instance().log_info("(Connection_Handler::retrive_client_id()) : Client connected: [" 
                                            + client_id + "\n");
        } else {
            Logger::get_instance().log_error("(Connection_Handler::retrive_client_id()) : Socket is not open; unable to retrieve client ID");
        }
    } catch (const std::exception& e) {
        Logger::get_instance().log_error(e);
    }
}

    
// read data asynchronously from the client
void Connection_Handler::do_read() {
    auto self = shared_from_this(); // capture shared ownership of the handler

    read_buffer.consume(read_buffer.size()); // reset the buffer

    boost::asio::async_read_until(
        connection_socket,
        read_buffer,
        '\n',
        [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
            self->handle_read(err, bytes_transferred); // handle the received data
        });
}


// handle the incoming message from the client
void Connection_Handler::handle_read(const boost::system::error_code& err, std::size_t bytes_transferred) {        
    if (!err) {
        // extract the message from the buffer
        std::istream stream(&read_buffer);
        std::string data;
        std::getline(stream, data);

        // print the received message
        if (data.empty()) {
            std::cout << "Received an empty message from client #" << unique_client_number << "\n";
            Logger::get_instance().log_info("(Connection_Handler::handle_read()) : Received an empty message from client #" + std::to_string(unique_client_number));
        } else {
            std::cout << "Client #" << unique_client_number << "> " << data << "\n";
            Logger::get_instance().log_info("(Connection_Handler::handle_read()) : Client #" + std::to_string(unique_client_number)  + "> " + data);
        }

        read_buffer.consume(bytes_transferred); // clear the buffer
        Logger::get_instance().log_debug("(Connection_Handler::handle_read()) : Buffer cleared");

        do_read(); // continue reading
    } else if (err == boost::asio::error::eof) {
        // handle client disconnection
        std::cout << "\nConnection closed by the client #" << unique_client_number << " : [" << client_id << "]\n";
        Logger::get_instance().log_info("(Connection_Handler::handle_read()) : Connection closed by the client #" + std::to_string(unique_client_number) + " : [" + client_id + "]");

        connection_socket.close();
        Logger::get_instance().log_info("(Connection_Handler::handle_read()) : Socket closed due to client disconnected");

        client_manager.remove_client(client_id);
            
        Logger::get_instance().log_info("(Connection_Handler::handle_read()) : Client #" + std::to_string(unique_client_number) 
                                         + " removed from the map. Current clients: " + std::to_string(client_manager.get_client_count()));
                      
    } else if (err == boost::asio::error::operation_aborted) {
        // handle aborted operation (e.g. server shut down)
        Logger::get_instance().log_error("(Connection_Handler::handle_read()) : Operation aborted for client #" + std::to_string(unique_client_number) + " : [" + client_id + "]");
        
    } else {
        // handle all other errors
        Logger::get_instance().log_error("(Connection_Handler::handle_read()) : Read error: " + err.message());
        connection_socket.close();
        Logger::get_instance().log_info("(Connection_Handler::handle_read()) : Socket closed");

        client_manager.remove_client(client_id);
        Logger::get_instance().log_debug("(Connection_Handler::handle_read()) : Client #" + std::to_string(unique_client_number) + " removed from the map");
    }
}