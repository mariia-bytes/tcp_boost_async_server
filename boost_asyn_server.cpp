#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

using namespace boost::asio;
using ip::tcp;

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

        auto self = shared_from_this();
        connection_socket.async_write_some(
            boost::asio::buffer(message),
            [self](const boost::system::error_code& err, std::size_t bytes_transferred) {
                if (!err) {
                    std::cout << "\nMessage sent to client: " << self->message << std::endl;
                } else {
                    std::cerr << "Write error: " << err.message() << std::endl;
                }
            });
        
        connection_socket.async_read_some(
            boost::asio::buffer(data, max_length),
            boost::bind(&Connection_Handler::handle_read,
                        shared_from_this(),
                        boost::placeholders::_1,
                        boost::placeholders::_2));
        /*
        boost::asio::async_write(
            connection_socket,
            boost::asio::buffer(message),
            boost::bind(&Connection_Handler::handle_write,
                        shared_from_this(),
                        boost::placeholders::_1,
                        boost::placeholders::_2));
        */
    }
    
    void handle_read(const boost::system::error_code& err, size_t bytes_transferred) {
        if (!err) {
            // Null-terminate the received data to handle it as a string
            data[bytes_transferred] = '\0';
            std::cout << "Received message: " << data << std::endl;

            // Clear the buffer to remove any leftover data
            std::fill(std::begin(data), std::end(data), 0);
        
            // Continue reading more data from the client
            connection_socket.async_read_some(boost::asio::buffer(data, max_length),
                                              boost::bind(&Connection_Handler::handle_read,
                                                          shared_from_this(),
                                                          boost::placeholders::_1,
                                                          boost::placeholders::_2));
        } else {
            std::cerr << "Read error: " << err.message() << std::endl;
            connection_socket.close();
        }
    }
    

    void handle_write(const boost::system::error_code& err, size_t bytes_transferred) {
        if (!err) {
            std::cout << "\nServer sent: " << message << std::endl;
        } else {
            std::cerr << "\nWrite error: " << err.message() << std::endl;
            connection_socket.close();
        }
    }

};


class Server {
private:
    tcp::acceptor server_acceptor;
    
    void start_accept() {
        auto& io_context = static_cast<boost::asio::io_context&>(server_acceptor.get_executor().context());

        Connection_Handler::pointer connection = Connection_Handler::create(io_context);

        server_acceptor.async_accept(connection->socket(),
                                    boost::bind(&Server::handle_accept, this, connection,
                                                boost::asio::placeholders::error));

        std::cout << "\nWaiting for clients connection..." << std::endl;
    }

public:
    Server(boost::asio::io_context& io_context, int port) 
        : server_acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
            start_accept();
    }

    void handle_accept(Connection_Handler::pointer connection, const boost::system::error_code& err) {
        if (!err) {
            connection->start();
        }
        start_accept();
    }
};


int main(int argc, char* argv[]) {
    // determine the port from command-line argument or use default
    unsigned short port = 55000; // default port
    if (argc > 1) {
        try {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port provided. Using default port 55000" 
                      << std::endl;
        }
    }
    
    
    try {
        boost::asio::io_context io_context;
        Server server(io_context, port);

        // handling signal for graceful shutdown
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const boost::system::error_code&, int) {
            std::cout << "\nShutting down the server..." << std::endl;
            io_context.stop();
        });

        io_context.run();
    
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}