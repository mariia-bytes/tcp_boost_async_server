#include "Client_Manager.h"
#include "Connection_Handler.h"

// constructor: Initializes the client manager and file writer
Client_Manager::Client_Manager(boost::asio::io_context& io_context)
    : file_writer(io_context) {
        Logger::get_instance().log_debug("Client Manager initialized");
}

// adds a new client if it doesn't already exist
void Client_Manager::add_client(const std::string& client_id, const boost::shared_ptr<Connection_Handler>& handler) {
    std::lock_guard<std::mutex> lock(clients_mutex);
       
    if (clients.find(client_id) == clients.end()) { // ensure client is not already in the map
        unsigned int client_number = generate_unique_id();
        clients[client_id] = {client_number, handler};
        file_writer.update_data(get_clients_snapshot());
        
        Logger::get_instance().log_debug("(Client_Manager::add_client()) : Client #" + std::to_string(client_number) + " added to the map: [" + client_id + "]");
        Logger::get_instance().log_debug("(Client_Manager::add_client()) : Current clients: " + std::to_string(clients.size()));

    } else {
        Logger::get_instance().log_debug("(Client_Manager::add_client()) : Client already exists: [" + client_id + "]");
    }      
}

// generates a new unique ID
unsigned int Client_Manager::generate_unique_id() {
    return ++client_id_counter;
}

// retrieves the client number for a given client ID
std::optional<unsigned int> Client_Manager::get_client_number(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex);

    auto it = clients.find(client_id);
    if (it != clients.end()) {
        return it->second.first; // return the client number
    }
    return std::nullopt;
}

// removes a client from the manager
void Client_Manager::remove_client(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex);

    // check if the client is in the map
    auto it = clients.find(client_id);
    if (it != clients.end()) {
        unsigned int client_number = it->second.first; // extract the unique ID
        clients.erase(it);
        Logger::get_instance().log_info("(Client_Manager::remove_client()): Client #" + std::to_string(client_number) + " removed from the map : ["  + client_id + "]");
    } else {
        Logger::get_instance().log_error("(Client_Manager::remove_client()): Attempted to remove non-existent client: [" + client_id + "]");
    }
    Logger::get_instance().log_debug("(Client_Manager::remove_client): Current clients: " + std::to_string(clients.size()));
}

// returns the current number of connected clients
size_t Client_Manager::get_client_count() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    return clients.size();
}

// creates and returns a snapshot of all clients
std::map<std::string, unsigned int> Client_Manager::get_clients_snapshot() {
    std::map<std::string, unsigned int> snapshot;
        
    for (const auto& [client_id, pair] : clients) {
        snapshot[client_id] = pair.first;
    }
        
    Logger::get_instance().log_info("Client_Manager::get_clients_snapshot()) : " + std::to_string(snapshot.size()) + " client added to snapshot");

    return snapshot;
}

// checks if the client is connected
// haven't used it so far but let it be here for now
bool Client_Manager::is_client_connected(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    return clients.find(client_id) != clients.end();
}