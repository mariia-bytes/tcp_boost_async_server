#include <iostream>
#include <boost/asio.hpp>
#include <string>

using namespace boost::asio;
using ip::tcp;

//default IP address and port
std::string ip_address = "127.0.0.1";
unsigned short port = 55000;

class Client {
public:
    Client(const std::string& ip_address, unsigned short port)
        : io_context_(), resolver_(io_context_), socket_(io_context_) {
        
        auto endpoints = resolver_.resolve(ip_address, std::to_string(port));
        // connect to the server
        boost::asio::connect(socket_, endpoints);

        std::cout << "\nConnection to " << ip_address << ":" << port << " established\n\n";

        // read server's greeting
        read_message();

        // run the I/O context to process asynchronous operations
        io_context_.run();
    }

    void send_message(const std::string& message) {
        const std::string msg = message + "\n";

        // asynchronously send the message to the server
        boost::asio::async_write(socket_, boost::asio::buffer(msg),
            [&](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    std::cerr << "Error sending message: " << ec.message() << std::endl;
                } else {
                    // after sending, immediately attempt to read the response
                    read_message();
                }
            });
    }

    void read_message() {
        auto buffer = std::make_shared<boost::asio::streambuf>();

        // asynchronously read data from the server
        boost::asio::async_read_until(socket_, *buffer, "\n",
            [this, buffer](const boost::system::error_code& ec, std::size_t) {
                if (!ec) {
                    std::istream stream(buffer.get());
                    std::string message;
                    std::getline(stream, message);
                    std::cout << "Server> " << message << std::endl;
                } else {
                    std::cerr << "Error reading message: " << ec.message() << std::endl;
                    if (ec == boost::asio::error::eof) {
                        std::cout << "Connection closed by the server.\n";
                        std::cout << "\nShutting down the client...\n";
                        close_connection();
                    }                   
                }
            });
    }

    void chat() {
        std::string user_input;
        while (true) {
            // ask user for the input
            std::cout << "YOU> ";
            std::getline(std::cin, user_input);

            if (user_input == "exit") {
                std::cout << "\nExiting the chat...\n";
                break;
            }

            // send the user input to the server
            send_message(user_input);

            // to perform non-blocking operations
            io_context_.run_one();
        }
    }

    void close_connection() {
        std::cout << "\nClosing the connection..." << std::endl;
        socket_.close();
    }

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket socket_;
};

int main(int argc, char* argv[]) {

    if (argc > 1) {
        try {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port provided. Using default port " << port << std::endl;
        }
    }

    try {
        Client client(ip_address, port);

        // srart chatting with the server
        client.chat();

        // close the connection when done
        client.close_connection();

    } catch (std::exception& excep) {
        std::cerr << "Exception: " << excep.what() << std::endl;
    }

    return 0;
}