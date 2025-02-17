#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include <boost/enable_shared_from_this.hpp>
#include "Client_Manager.h"

/**
 * @class Connection_Handler
 * @brief Handles individual client connections.
 *
 * The Connection_Handler class represents a single client's connection to the server.
 * It is responsible for sending and receiving messages, maintaining the client’s unique identifier,
 * and interacting with the Client_Manager to track active connections.
 * The class supports asynchronous communication using Boost.Asio, ensuring efficient 
 * non-blocking operations. It also handles client disconnections gracefully.
 *
 * @note This class is designed to be used in a networked asynchronous environment.
 */
class Connection_Handler : public boost::enable_shared_from_this<Connection_Handler> {
private: 
    boost::asio::ip::tcp::socket connection_socket; /**< Socket for client connection */
    std::string client_id; /**< Unique identifier for the client (format: "IP:port") */
    Client_Manager& client_manager; /**< Reference to Client_Manager for tracking clients  */
    unsigned int unique_client_number; /**< Unique number assigned to the client by Client_Manager */
    std::string message = "Hello from Server! Your unique ID is #"; /**< Initial greeting message */
    boost::asio::streambuf read_buffer; /**< Buffer to read client messages */

public:
    typedef boost::shared_ptr<Connection_Handler> pointer; /**< Shared pointer type alias */

public:
    /**
     * @brief Constructor.
     * @param io_context The Boost ASIO io_context.
     * @param maganer Reference to the Client_Manager.
     */
    Connection_Handler(boost::asio::io_context& io_context, Client_Manager& manager);
    
    /**
     * @brief Destructor.
     */
    ~Connection_Handler();
        
    /**
     * @brief Factory method to create a shared pointer instance.
     * @param io_context The Boost ASIO ip_context.
     * @param manager Reference to the Client_Manager.
     * @return A shared pointer to a new Connection_Handler instance.
     */
    static pointer create(boost::asio::io_context& io_context, Client_Manager& manager);

    /**
     * @brief Accessor for the socket.
     * @return Reference to the connection socket.
     */
    boost::asio::ip::tcp::socket& socket();   

    /**
     * @brief Start handling connection: send greeting and read client
     */
    void start();

    /**
     * @brief Retrive client ID (IP:port).
     */
    void retrieve_client_id(); 

private:
    /**
     * @brief Read data asynchronously from clinet.
     */
    void do_read();

    /**
     * @brief Handle received client message.
     * @param err The Boost ASIO error code.
     * @param bytes_transferred The number of bytes transferred.
     */
    void handle_read(const boost::system::error_code& err, std::size_t bytes_transferred);
};

#endif // CONNECTION_HANDLER_H