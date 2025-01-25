#include <iostream>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <thread>
#include <algorithm>
#include <string>
#include <map>
#include <mutex>

unsigned short port = 55000; // default port
std::string ip_address = "127.0.0.1"; // default IP address

using namespace boost::asio;
using ip::tcp;

class Connection_Handler;



/***** CLASS CLIENT MANAGER *************************************************************************/
class Client_Manager {
private:
    std::map<std::string, boost::shared_ptr<Connection_Handler>> clients; // map of connected clients
    std::mutex clients_mutex; // protection of the clients map
   
public:
    // add a new client
    void add_client(const std::string& client_id, const boost::shared_ptr<Connection_Handler>& handler) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[client_id] = handler;
        std::cout << "\tClient added: " << client_id << "\n";
        std::cout << "\tCurrent clients: " << clients.size() << "\n";
    }

    // remove a client
    void remove_client(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(clients_mutex);

        // check if the client is in the map
        auto it = clients.find(client_id);
        if (it != clients.end()) {
            clients.erase(it);
            std::cout << "\tClient removed from the map: " << client_id << "\n";
        } else {
            std::cerr << "\n\tAttempted to remove a non-existent client: " << client_id << "\n";
        }
        std::cout << "\tCurrent clients: " << clients.size() << "\n";
    }
    
    // get the current number of clients
    size_t get_client_count() {
        std::lock_guard<std::mutex> lock(clients_mutex);
        return clients.size();
    }

    // check if the client is connected
    // haven't used it so far but let it be here for now
    bool is_client_connected(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        return clients.find(client_id) != clients.end();
    }
};
/*****************************************************************************************************/



/************ CLASS CONNECTION HANDLER *********************************************************************/
class Connection_Handler : public boost::enable_shared_from_this<Connection_Handler> {
private: 
    tcp::socket connection_socket;
    std::string client_id; // unique identifier for the clien
    Client_Manager& client_manager; 
    std::string message = "Hello from Server!\n"; // greeting message to the server
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

        retrive_client_id();
        client_manager.add_client(client_id, shared_from_this());

        // send an initial message
        connection_socket.async_write_some(
            boost::asio::buffer(message),
            [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                if (!err) {
                    std::cout << "\nMessage sent to client: " << self->message;
                } else {
                    std::cerr << "Write error: " << err.message() << "\n";
                }
            });
        do_read();
    }

    void retrive_client_id() {
        try {
            client_id = connection_socket.remote_endpoint().address().to_string() 
                        + ":" + std::to_string(connection_socket.remote_endpoint().port());
            std::cout << "\nClient connected: " << client_id << "\n";
        } catch (const std::exception& e) {
            std::cerr << "\nError retrieving client information: "
                      << e.what() << "\n";
        }
    }

    ~Connection_Handler() {
        if (connection_socket.is_open()) {
            boost::system::error_code ec;
            connection_socket.cancel(ec); // Cancel pending operations
            if (ec) {
                std::cerr << "Error canceling socket: " << ec.message() << "\n";
            }
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
                std::cout << "Received an empty message from client.\n";
            } else {
                std::cout << "Client> " << data << "\n";
            }

            // clear the buffer
            read_buffer.consume(bytes_transferred);

            // continue reading
            do_read();
        } else if (err == boost::asio::error::eof) {
            // client disconnected gracefully
            std::cout << "\nConnection closed by the client: " << client_id << "\n";
            connection_socket.close();
            client_manager.remove_client(client_id);
            std::cout << "Current clients: " << client_manager.get_client_count() << "\n";
            
        } else if (err == boost::asio::error::operation_aborted) {
            // (possibly) the server shut down
            std::cerr << "Operation aborted for client: " << client_id << "\n";
        /*
        } else if (err == boost::asio::error::resouce_unavailable_try_again) {
            // temporary issue, log and retry
            std::cerr << "Temporary resource issue for client: " << client_id << "\n";
            do_read();
        */
        } else {
            // all other errors are treated as non-recoverable
            std::cerr << "Read error: " << err.message() << "\n";
            connection_socket.close();
            client_manager.remove_client(client_id);
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
            std::cout << "\nWaiting for a client to connect on " << ip_address << ":" << port << "...\n";
        }        
    }

public:
    Server(boost::asio::io_context& io_context, const std::string& ip_address, int port) 
        : server_acceptor(io_context, tcp::endpoint(boost::asio::ip::make_address(ip_address), port)) {
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
        Server server(io_context, ip_address, port);

        // handling signal for graceful shutdown
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const boost::system::error_code&, int) {
            std::cout << "\n\nShutting down the server..." << std::endl;
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
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}