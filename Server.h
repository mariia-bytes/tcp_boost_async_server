#ifndef SERVER_H
#define SERVER_H

#include "Client_Manager.h"
#include "Connection_Handler.h"

extern std::string ip_address;
extern unsigned short port;

/**
 * @class Server
 * @brief Represents a TCP server that manages client connections.
 */
class Server {
private:
    boost::asio::ip::tcp::acceptor server_acceptor; /**< Acceptor to listen for incoming connections */
    Client_Manager client_manager; /**< Manages connected clients */
    std::atomic<bool> is_waiting = true; /**< Flag to control print "waiting..." message display */

public:
    /**
     * @brief Constructor.
     * @param io_context The Boost ASIO io_context.
     * @param ip_address The IP address to bind the server to.
     * @param port The port number to listen on.
     */
    Server(boost::asio::io_context& io_context, const std::string& ip_address, const int port);

    /**
     * @brief Destructor.
     */
    ~Server();

    /**
     * @brief Handle acceptance of a new client connection.
     * @param connection Shared pointer to the new Connection_Handler.
     * @param err Error code indicating the status of the acceptance.
     */
    void on_client_connected(Connection_Handler::pointer connection, const boost::system::error_code& err);

private:
    /**
     * @brief Initiates asynchronous acceptance for new client connections.
     */
    void start_connections();
};

#endif // SERVER_H