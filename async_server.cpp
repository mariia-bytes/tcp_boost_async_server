#include <iostream>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <thread>
#include <algorithm>
#include <string>
#include <map>
#include <mutex>

unsigned short port = 55000; // default port
std::string ip_address = "127.0.0.1"; // default IP address

using namespace boost::asio;
using ip::tcp;


/************ CLASS CONNECTION HANDLER *********************************************************************/
class Connection_Handler : public boost::enable_shared_from_this<Connection_Handler> {
private: 
    tcp::socket connection_socket;
    std::string client_id; // unique adentifier for the client
    // Server& server;
    std::string message = "Hello from Server!\n"; // greeting message to the server
    boost::asio::streambuf read_buffer; // dynamic buffer to read from the socket
public:
    typedef boost::shared_ptr<Connection_Handler> pointer;


public:
    Connection_Handler(boost::asio::io_context& io_context) 
        : connection_socket(io_context) {}

    // get client unique identifier
    std::string get_client_id() const {
        return client_id;
    }
        
    // create a shared pointer
    static pointer create(boost::asio::io_context& io_context) {
        return pointer(new Connection_Handler(io_context));
    }

    // access socket
    tcp::socket& socket() {
        return connection_socket;
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

    void start() {
        // capture shared ownership of the handler
        auto self = shared_from_this();

        retrive_client_id();

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

            read_buffer.consume(bytes_transferred);
            do_read();
        } else if (err == boost::asio::error::eof) {
            std::cout << "\nConnection closed by the client: " << client_id << "\n";
            // std::cout << "Current clients: " << need to make something
            connection_socket.close();
            // server.remove_client(client_id); // notify the server
        } else {
            std::cerr << "Read error: " << err.message() << "\n";
            connection_socket.close();
            // server.remove_client(client_id); // notify the server
        }
    }
};
/*********************************************************************************************************/

/************ CLASS SERVER *******************************************************************************/
class Server {
private:
    tcp::acceptor server_acceptor;
    std::map<std::string, Connection_Handler::pointer> clients; // map of connected clients
    std::mutex clients_mutex; // to protect clients map
    std::atomic<bool> is_waiting = true; // flag to control print "waiting..."
    
    void start_accept() {
        auto& io_context = static_cast<boost::asio::io_context&>(server_acceptor.get_executor().context());
        
        Connection_Handler::pointer connection = Connection_Handler::create(io_context);

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
            std::string client_id = connection->get_client_id();
            // lock before modifying the map
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients[client_id] = connection;
            std::cout << "Current clients: " << clients.size() << "\n";
        }
        start_accept();
    }

    // remove client when it's disconnected
    void remove_client(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(client_id);
        std::cout << "\nClient disconnected: " << client_id << "\n";
        std::cout << "Current clients: " << clients.size() << "\n";
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
            std::cout << "\nShutting down the server..." << std::endl;
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