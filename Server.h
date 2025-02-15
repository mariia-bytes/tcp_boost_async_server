#ifndef SERVER_H
#define SERVER_H

#include "Client_Manager.h"
#include "Connection_Handler.h"

extern std::string ip_address;
extern unsigned short port;

class Server {
private:
    boost::asio::ip::tcp::acceptor server_acceptor;
    Client_Manager client_manager; 
    std::atomic<bool> is_waiting = true; // flag to control print "waiting..."
    
    void start_accept();

public:
    Server(boost::asio::io_context& io_context, const std::string& ip_address, const int port);
    ~Server() = default;

    void handle_accept(Connection_Handler::pointer connection, const boost::system::error_code& err);
};

#endif // SERVER_H