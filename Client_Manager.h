#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <atomic>
#include <optional>
#include <boost/shared_ptr.hpp>
#include "Async_File_Writer.h"

class Connection_Handler;

/**
 * @class Client_Manager
 * @brief Manages client connections, providing functionality to add, remove, and track clients.
 *
 * The Client_Manager class maintains a thread-safe map of connected clients, each identified by a unique ID.
 * It also provides methods to generate unique client IDs, retrieve client information, and check connection status.
 * Additionally, it logs client-related events and updates client data using an asynchronous file writer.
 *
 * @note This class is designed to be used in an asynchronous networked environment.
 */
class Client_Manager {
private:
    std::map<std::string, std::pair<unsigned int, boost::shared_ptr<Connection_Handler>>> clients; /**< Map of connected clients (client ID -> (unique client number, connection handler)) */
    std::mutex clients_mutex; /**< Mutex to protect access to the clients map */
    std::atomic<unsigned int> client_id_counter {0}; /**< Atomic counter for generating unique client numbers */
    Async_File_Writer file_writer; /**< Asynchronous file write for logging client data */
   
public:
    /**
     * @brief Constructor
     * @param io_context The Boost ASIO io_context.
     */
    Client_Manager(boost::asio::io_context& io_context);

    /**
     * @brief Destructor
     */
    ~Client_Manager() = default;

    /**
     * @brief Adds a new client to the Manager
     * @param client_id The unique identifier for the client.
     */
    void add_client(const std::string& client_id, const boost::shared_ptr<Connection_Handler>& handler);

    // generate a new unique ID
    unsigned int generate_unique_id();

    // get client number by client_id
    std::optional<unsigned int> get_client_number(const std::string& client_id);

    // remove a client
    void remove_client(const std::string& client_id);
    
    // get the current number of clients
    size_t get_client_count();

    std::map<std::string, unsigned int> get_clients_snapshot();

    // check if the client is connected
    // haven't used it so far but let it be here for now
    bool is_client_connected(const std::string& client_id);
};

#endif // CLIENT_MANAGER_H