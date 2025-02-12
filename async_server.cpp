// everything I/O related
#include <iostream>
#include <fstream>

// everything boost related
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

// everything thread-related
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

// everything data structures related
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

// additional
#include <optional>
#include <chrono>

// everything logs related
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

// logging macros
#define LOG_INFO(msg) BOOST_LOG_TRIVIAL(info) << msg
#define LOG_ERROR(msg) BOOST_LOG_TRIVIAL(error) << msg
#define LOG_DEBUG(msg) BOOST_LOG_TRIVIAL(debug) << msg

// global variables for default
unsigned short port = 55000; // default port
std::string ip_address = "127.0.0.1"; // default IP address
std::string file_path = "clients_log.txt"; // default file path to write clients

using namespace boost::asio;
using ip::tcp;

class Connection_Handler;
// void init_logging();

void init_logging() {
    boost::log::add_file_log(
        boost::log::keywords::file_name = "server_log_%N.log",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024, // max 10 MB per file
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::format = "[%TimeStamp%] [%Severity%]: %Message%"
    );

    boost::log::add_common_attributes();
    boost::log::core::get()->set_filter(
        boost::log::trivial::severity >= boost::log::trivial::debug
    );
}


/***** CLASS Async_File_Writer ***********************************************************************/
class Async_File_Writer {
private:
    boost::asio::io_context& io_context;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    std::string file_path;
    std::set<std::pair<std::string, unsigned int>> clients_snapshot;
    std::set<std::pair<std::string, unsigned int>> written_clients;
    std::mutex snapshot_mutex; // protecs clients snapshot    

public:
    Async_File_Writer(boost::asio::io_context& context, const std::string& path) 
        : io_context(context), strand(context.get_executor()), file_path(path) {}

    ~Async_File_Writer() = default;

    void update_data(const std::map<std::string, unsigned int>& clients) {
        std::set<std::pair<std::string, unsigned int>> new_clients;
        {
            std::lock_guard<std::mutex> lock(snapshot_mutex);
        
            LOG_DEBUG("(Async_File_Writer::update_data) : Current snapshot size: " << clients_snapshot.size());
            // std::cout << "\n\tDEBUG: Updating data. Current snapshot size: " << clients_snapshot.size() << "\n";

            for (const auto& client : clients) {
            std::pair<std::string, unsigned int> client_entry = {client.first, client.second};

                // Only add clients that are NOT already written
                if (written_clients.find(client_entry) == written_clients.end()) {
                    new_clients.insert(client_entry);
                    clients_snapshot.insert(client_entry);
                }
            }
        }

        // post the file-writing operation to the strand
        if (!new_clients.empty()) {
            boost::asio::post(strand, [this, new_clients]() { write_to_file(new_clients); });
        }
    }

private:
    void write_to_file(const std::set<std::pair<std::string, unsigned int>>& new_clients) {
        if (new_clients.empty()) return;

        std::ofstream file(file_path, std::ios::app);  // open file in append mode
        if (file.is_open()) {
            LOG_INFO("(Async_File_Writer::write_to_file()) : File is open {" << file_path << "}");
            for (const auto& [client_id, client_number] : new_clients) {
                file << "Client #" << client_number << " : [" << client_id << "]\n";
                LOG_INFO("(Async_File_Writer::write_to_file()) : Client info {Client #" << client_number << " : [" << client_id << "]} is written to file " << file_path);
            }

            std::lock_guard<std::mutex> lock(snapshot_mutex);
            written_clients.insert(new_clients.begin(), new_clients.end());  // mark clients as logged
            LOG_INFO("(Async_File_Writer::write_to_file()): Mark client as logged");
        } else {
            LOG_ERROR("(Async_File_Writer::write_to_file()) : Failed to open the file: " << file_path);
            // std::cerr << "Failed to open the file: " << file_path << "\n";
        }    
    }
};

/*****************************************************************************************************/

/***** CLASS CLIENT MANAGER **************************************************************************/
class Client_Manager {
private:
    std::map<std::string, std::pair<unsigned int, boost::shared_ptr<Connection_Handler>>> clients; // map of connected clients
    std::mutex clients_mutex; // protection of the clients map
    std::atomic<unsigned int> client_id_counter {0}; // unique client number counter
    Async_File_Writer file_writer;
   
public:
    Client_Manager(boost::asio::io_context& io_context, const std::string& file_path)
        : file_writer(io_context, file_path) {}

    // add a new client
    void add_client(const std::string& client_id, const boost::shared_ptr<Connection_Handler>& handler) {
        std::lock_guard<std::mutex> lock(clients_mutex);
       
        if (clients.find(client_id) == clients.end()) { // ensure client is not already in the map
            unsigned int client_number = generate_unique_id();
            clients[client_id] = {client_number, handler};
            file_writer.update_data(get_clients_snapshot());
            LOG_DEBUG("(Client_Manager::add_client()) : Client #" << client_number << " added to the map: [" << client_id << "]");
            // std::cout << "\tDEBUG (Client_Manager::add_client()): Client #" << client_number << " added to the map: [" << client_id << "]\n";
            LOG_DEBUG("(Client_Manager::add_client()) : Current clients: " << clients.size());
            // std::cout << "\tDEBUG (Client_Manager::add_client()): Current clients: " << clients.size() << "\n";

        } else {
            LOG_DEBUG("(Client_Manager::add_client()) : Client already exists: " << client_id);
            // std::cout << "\tDEBUG (Client_Manager::add_client()): Client already exists: " << client_id << "\n";
        }      
    }

    // generate a new unique ID
    unsigned int generate_unique_id() {
        return ++client_id_counter;
    }

    // get client number by client_id
    std::optional<unsigned int> get_client_number(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(clients_mutex);

        auto it = clients.find(client_id);
        if (it != clients.end()) {
            return it->second.first; // return the client number
        }
        return std::nullopt;
    }

    // remove a client
    void remove_client(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(clients_mutex);

        // check if the client is in the map
        auto it = clients.find(client_id);
        if (it != clients.end()) {
            unsigned int client_number = it->second.first; // extract the unique ID
            clients.erase(it);
            // file_writer.update_data(get_clients_snapshot());
            LOG_DEBUG("(Client_Manager::remove_client()): Client #" << client_number << " removed : ["  << client_id << "]");
            // std::cout << "\tDEBUD (Client_Manager::remove_client()): Client #" << client_number << " removed : ["  << client_id << "]\n";
        } else {
            LOG_ERROR("(Client_Manager::remove_client()): Attempted to remove a non-existent client: " << client_id);
            // std::cerr << "\n\tAttempted to remove a non-existent client: " << client_id << "\n";
        }
        LOG_DEBUG("(Client_Manager::remove_client): Current clients: " << clients.size());
        // std::cout << "\tDEBUG (Client_Manager::remove_client): Current clients: " << clients.size() << "\n";
    }
    
    // get the current number of clients
    size_t get_client_count() {
        std::lock_guard<std::mutex> lock(clients_mutex);
        return clients.size();
    }

    std::map<std::string, unsigned int> get_clients_snapshot() {
        std::map<std::string, unsigned int> snapshot;

        /*
        for (const auto& [client_id, pair] : clients) {
            unsigned int client_number = pair.first;

            auto it = file_writer.clients_snapshot.find({client_id, client_number});
            if (it == file_writer.clients_snapshot.end()) {
                snapshot[client_id] = client_number;
            }
        }
        */
        
        for (const auto& [client_id, pair] : clients) {
            snapshot[client_id] = pair.first;
        }
        

        return snapshot;
    }

    // check if the client is connected
    // haven't used it so far but let it be here for now
    bool is_client_connected(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        return clients.find(client_id) != clients.end();
    }
};
/*****************************************************************************************************/



/************ CLASS CONNECTION HANDLER ***************************************************************/
class Connection_Handler : public boost::enable_shared_from_this<Connection_Handler> {
private: 
    tcp::socket connection_socket;
    std::string client_id; // unique identifier for the client (IP:port)
    Client_Manager& client_manager; 
    unsigned int unique_client_number; // client number assigned by Client_Manager
    std::string message = "Hello from Server! Your unique ID is #"; // message to clients
    boost::asio::streambuf read_buffer; // dynamic buffer to read from the socket
public:
    typedef boost::shared_ptr<Connection_Handler> pointer;


public:
    Connection_Handler(boost::asio::io_context& io_context, Client_Manager& manager) 
        : connection_socket(io_context), client_manager(manager) {}
        
    // create a shared pointer
    static pointer create(boost::asio::io_context& io_context, Client_Manager& manager) {
        return pointer(new Connection_Handler(io_context, manager));
    }

    // access socket
    tcp::socket& socket() {
        return connection_socket;
    }   

    void start() {
        // capture the ownership
        auto self = shared_from_this();

        // retrieve the client ID (IP:port)
        retrieve_client_id();

        // retrieve or add the client in the Client_Manager
        auto client_number_opt = client_manager.get_client_number(client_id);

        if (!client_number_opt) {
            // if the client ID is not yet registered, add it
            client_manager.add_client(client_id, self);
            client_number_opt = client_manager.get_client_number(client_id);
        }

        unique_client_number = client_number_opt.value();

        message += std::to_string(unique_client_number) + "\n";

        // send initial message
        connection_socket.async_write_some(
            boost::asio::buffer(message),
            [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                if (!err) {
                    std::cout << "\nMessage sent to client #" << self->unique_client_number << ": " << self->message;
                    LOG_INFO("(Connection_Handler::start()) : Message sent to client #" << self->unique_client_number << ": " << self->message);
                } else {
                    std::cerr << "Write error: " << err.message() << "\n";
                    LOG_ERROR("(Connection_Handler::start()) : Write error: " << err.message());
                }
            });
        do_read();
    }

    void retrieve_client_id() {
        try { 
            if (connection_socket.is_open()) {
                auto endpoint = connection_socket.remote_endpoint();
                client_id = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
                std::cout << "\nClient connected: " << client_id << "\n";
                LOG_INFO("(Connection_Handler::retrive_client_id()) : Client connected: " << client_id);
            } else {
                std::cerr << "\nSocket is not open; unable to retrieve client ID\n";
                LOG_ERROR("(Connection_Handler::retrive_client_id()) : Socket is not open; unable to retrieve client ID");
            }
        } catch (const std::exception& e) {
            std::cerr << "\nError retrieving client information: " << e.what() << "\n";
            LOG_ERROR("(Connection_Handler::retrive_client_id()) : Error retrieving client information: " << e.what());
        }
    }

    ~Connection_Handler() {
        try {      
            if (connection_socket.is_open()) {
                boost::system::error_code ec;
                connection_socket.close(ec);
                if (ec) {
                    std::cerr << "\nSocket close error: " << ec.message() << "\n";
                    LOG_ERROR("(~Connection_Handler) : Socket close error: " << ec.message());
                }
            }

        } catch (const std::exception& e) {
            std::cerr << "Error in Connection_Handler destructor: " << e.what() << "\n";
            LOG_ERROR("(~Connection_Handler) : Error in Connection_Handler destructor: " << e.what());
        }

        /* 
        // can't say why I don't have to do it but the valgrind tells me I don't
        // client_manager.remove_client(client_id);
        if (!client_id.empty()) {
            client_manager.remove_client(client_id);
        }
        std::cout << "Connection_Handler destroyed for client: " << client_id << "\n";
        */
    }
    

private:
    void do_read() {
        // capture shared ownership of the handler
        auto self = shared_from_this();

        // reset the buffer
        read_buffer.consume(read_buffer.size());

        boost::asio::async_read_until(
            connection_socket,
            read_buffer,
            '\n',
            [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                self->handle_read(err, bytes_transferred);
            });
    }

    void handle_read(const boost::system::error_code& err, std::size_t bytes_transferred) {        
        if (!err) {
           // extract from the buffer
            std::istream stream(&read_buffer);
            std::string data;
            std::getline(stream, data);

            // print the received message
            if (data.empty()) {
                std::cout << "Received an empty message from client #" << unique_client_number << "\n";
                LOG_INFO("(Connection_Handler::handle_read()) : Received an empty message from client #" << unique_client_number);
            } else {
                std::cout << "Client #" << unique_client_number << "> " << data << "\n";
                LOG_INFO("(Connection_Handler::handle_read()) : Client #" << unique_client_number  << "> " << data);
            }

            // clear the buffer
            read_buffer.consume(bytes_transferred);
            LOG_INFO("(Connection_Handler::handle_read()) : Buffer cleared");

            // continue reading
            do_read();
        } else if (err == boost::asio::error::eof) {
            // client disconnected gracefully
            std::cout << "\nConnection closed by the client #" << unique_client_number << " : [" << client_id << "]\n";
            LOG_INFO("(Connection_Handler::handle_read()) : Connection closed by the client #" << unique_client_number << " : [" << client_id << "]");
            connection_socket.close();
            LOG_INFO("(Connection_Handler::handle_read()) : Socket closed");

            client_manager.remove_client(client_id);
            LOG_INFO("(Connection_Handler::handle_read()) : Client #" << unique_client_number << " removed from the map");
            std::cout << "Current clients: " << client_manager.get_client_count() << "\n";
            LOG_INFO("(Connection_Handler::handle_read()) : Current clients: " << client_manager.get_client_count());
            
        } else if (err == boost::asio::error::operation_aborted) {
            // (possibly) the server shut down
            LOG_ERROR("(Connection_Handler::handle_read()) : Operation aborted for client #" << unique_client_number << " : [" << client_id << "]");
        /*
        } else if (err == boost::asio::error::resouce_unavailable_try_again) {
            // temporary issue, log and retry
            std::cerr << "Temporary resource issue for client: " << client_id << "\n";
            do_read();
        */
        } else {
            // all other errors are treated as non-recoverable
            LOG_ERROR("(Connection_Handler::handle_read()) : Read error: " << err.message());
            connection_socket.close();
            LOG_INFO("(Connection_Handler::handle_read()) : Socket closed");
            client_manager.remove_client(client_id);
            LOG_INFO("(Connection_Handler::handle_read()) : Client #" << unique_client_number << " removed from the map");
        }
    }
};
/*********************************************************************************************************/

/************ CLASS SERVER *******************************************************************************/
class Server {
private:
    tcp::acceptor server_acceptor;
    Client_Manager client_manager; 
    std::atomic<bool> is_waiting = true; // flag to control print "waiting..."
    
    void start_accept() {
        boost::asio::io_context& io_context = static_cast<boost::asio::io_context&>(server_acceptor.get_executor().context());
        
        auto connection = Connection_Handler::create(io_context, client_manager);

        server_acceptor.async_accept(
            connection->socket(),
            [this, connection](const boost::system::error_code& error) {
                handle_accept(connection, error);
            });

        if (is_waiting.exchange(false)) { // atomic check and set
            std::cout << "\nWaiting for clients to connect on " << ip_address << ":" << port << "...\n";
            LOG_INFO("(Server::start_accept()) : Waiting for clients to connect on " << ip_address << ":" << port << "...");
        }        
    }

public:
    Server(boost::asio::io_context& io_context, const std::string& ip_address, int port, const std::string& file_path) 
        : server_acceptor(io_context, tcp::endpoint(boost::asio::ip::make_address(ip_address), port)),
          client_manager(io_context, file_path) {
            start_accept();
    }

    void handle_accept(Connection_Handler::pointer connection, const boost::system::error_code& err) {
        if (!err) {
            connection->start();
        }
        start_accept();
    }
};
/*****************************************************************************************************/







int main(int argc, char* argv[]) {
    init_logging();

    // determine the port from command-line argument or use default    
    if (argc > 1) {
        try {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port provided. Using default port 55000\n";
        }
    }

    

    try {
        boost::asio::io_context io_context;

        // create a thread pool
        unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> thread_pool;

        // create and start the server
        Server server(io_context, ip_address, port, file_path);

        // handling signal for graceful shutdown
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const boost::system::error_code&, int) {
            std::cout << "\n\nShutting down the server..." << std::endl;
            LOG_INFO("main() : Shutting down the server...");
            io_context.stop();
        });

        // post work to the thread pool
        for (unsigned int i = 0; i < num_threads; i++) {
            thread_pool.emplace_back([&io_context]() { io_context.run(); });
        }

        // wait till all threads are done
        for (auto& thread : thread_pool) {
            thread.join();
        }
    
    } catch (std::exception& e) {
        LOG_ERROR("main() : Exception: " << e.what());
    }

    return 0;
}