#include "Server.h"
#include <iostream>

// export std::string file_path;

Server::Server(boost::asio::io_context& io_context, const std::string& ip_address, const int port, const std::string& file_path) 
        : server_acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(ip_address), port)),
          client_manager(io_context, file_path) {
            Logger::get_instance().log_info("Server initialized on " + ip_address + " : " + std::to_string(port));
            start_accept();
    }

void Server::start_accept() {
        boost::asio::io_context& io_context = static_cast<boost::asio::io_context&>(server_acceptor.get_executor().context());
        
        auto connection = Connection_Handler::create(io_context, client_manager);

        server_acceptor.async_accept(
            connection->socket(),
            [this, connection](const boost::system::error_code& error) {
                handle_accept(connection, error);
            });

        if (is_waiting.exchange(false)) { // atomic check and set
            std::cout << "\nWaiting for clients to connect on [" << ip_address << " : " << port << "] ...\n";
            Logger::get_instance().log_info("(Server::start_accept()) : Waiting for clients to connect on [" + ip_address + " : " + std::to_string(port) + "] ...");
        }        
    }


    

    void Server::handle_accept(Connection_Handler::pointer connection, const boost::system::error_code& err) {
        if (!err) {
            connection->start();
        }
        start_accept();
    }