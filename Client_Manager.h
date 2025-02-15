#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <atomic>
#include <optional>
#include <boost/shared_ptr.hpp>
#include "Async_File_Writer.h"

class Connection_Handler;

class Client_Manager {
private:
    std::map<std::string, std::pair<unsigned int, boost::shared_ptr<Connection_Handler>>> clients; // map of connected clients
    std::mutex clients_mutex; // protection of the clients map
    std::atomic<unsigned int> client_id_counter {0}; // unique client number counter
    Async_File_Writer file_writer;
   
public:
    Client_Manager(boost::asio::io_context& io_context);
    ~Client_Manager() = default;

    // add a new client
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