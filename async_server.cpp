#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <thread>
#include <algorithm>
#include <string>

unsigned short port = 55000; // default port
std::string ip_address = "127.0.0.1"; // default IP address

using namespace boost::asio;
using ip::tcp;


/************ CLASS CONNECTION HANDLER *********************************************************************/
class Connection_Handler : public boost::enable_shared_from_this<Connection_Handler> {
private: 
    tcp::socket connection_socket;
    std::string message = "Hello from Server!\n";
    enum { max_length = 1024 };
    char data[max_length];

public:
    typedef boost::shared_ptr<Connection_Handler> pointer;

    Connection_Handler(boost::asio::io_context& io_context) 
        : connection_socket(io_context) {}

    // create a shared pointer
    static pointer create(boost::asio::io_context& io_context) {
        return pointer(new Connection_Handler(io_context));
    }

    // access socket
    tcp::socket& socket() {
        return connection_socket;
    }

    void start() {
        // capture shared ownership of the handler
        auto self = shared_from_this();

        // send a message to the client asynchronously
        connection_socket.async_write_some(
            boost::asio::buffer(message),
            [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                if (!err) {
                    std::cout << "\nMessage sent to client: " << self->message;
                } else {
                    std::cerr << "Write error: " << err.message() << "\n";
                }
            });
        
        // read data from the client asynchronously
        connection_socket.async_read_some(
            boost::asio::buffer(data, max_length),
            [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                if (!err) {
                    if (bytes_transferred < max_length) {
                        self->data[bytes_transferred] = '\0';
                    }
                    std::cout << "Client> " << self->data << "\n";

                    // clear the buffer
                    std::fill(std::begin(self->data), std::end(self->data), 0);

                    // continue reading more data from the client
                    self->connection_socket.async_read_some(
                        boost::asio::buffer(self->data, max_length),
                        [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                            self->handle_read(err, bytes_transferred);
                        });
                } else if (err == boost::asio::error::eof) {
                    std::cout << "Connection closed by the client\n";
                    self->connection_socket.close();
                } else {
                    std::cerr << "Read error: " << err.message() << "\n";
                    self->connection_socket.close();
                }
            });
    }

    void handle_read(const boost::system::error_code& err, size_t bytes_transferred) {
        if (!err) {
            // Null-terminate the received data to handle it as a string
            if (bytes_transferred < max_length) {
                data[bytes_transferred] = '\0';
            }
            std::cout << "Client> " << data << "\n";

            // Clear the buffer to remove any leftover data
            std::fill(std::begin(data), std::end(data), 0);
        
            // Continue reading more data from the client
            connection_socket.async_read_some(boost::asio::buffer(data, max_length),
                                              boost::bind(&Connection_Handler::handle_read,
                                                          shared_from_this(),
                                                          boost::placeholders::_1,
                                                          boost::placeholders::_2));
        } else if (err == boost::asio::error::eof) {
            // Handle EOF gracefully, which indicates the connection was closed by the client
            std::cout << "Connection closed by the client\n";
            connection_socket.close();
        
        } else {
            std::cerr << "Read error: " << err.message() << "\n";
            connection_socket.close();
        }
    }
};
/*********************************************************************************************************/

/************ CLASS SERVER *******************************************************************************/
class Server {
private:
    tcp::acceptor server_acceptor;
    bool is_waiting = true; // flag to control print "waiting..."
    
    void start_accept() {
        auto& io_context = static_cast<boost::asio::io_context&>(server_acceptor.get_executor().context());
        
        Connection_Handler::pointer connection = Connection_Handler::create(io_context);

        server_acceptor.async_accept(connection->socket(),
                                    boost::bind(&Server::handle_accept, this, connection,
                                                boost::asio::placeholders::error));

        if (is_waiting) {
            std::cout << "\nWaiting for a client to connect on " << ip_address << ":" << port << "...\n";
            is_waiting = false;
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
            std::cout << "\nShutting down the server..." << std::endl;
            io_context.stop();
        });

        // post work to the thread pool
        for (unsigned int i = 0; i < num_threads; i++) {
            thread_pool.emplace_back([&io_context]() { io_context.run(); });
        }

        for (auto& thread : thread_pool) {
            thread.join();
        }
    
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}