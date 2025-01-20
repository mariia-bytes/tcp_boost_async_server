#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;

class Client {
public:
    Client(const std::string& ip_address, unsigned short port)
        : io_context_(), resolver_(io_context_), socket_(io_context_) {
        // Resolve the server's address and port
        auto endpoints = resolver_.resolve(ip_address, std::to_string(port));

        // Connect to the server
        boost::asio::connect(socket_, endpoints);

        std::cout << "\nConnection to " << ip_address << ":" << port << " established" << std::endl;
    }
    void send_message(boost::asio::ip::tcp::socket& socket, const std::string message) {
        const std::string msg = message + "\n";
        boost::asio::write(socket, boost::asio::buffer(msg));
    }

    void sendMessage(const std::string& message) {
        send_message(socket_, message);
        std::cout << "\nClient sent: " << message << std::endl;
    }

    std::string read_message(boost::asio::ip::tcp::socket& socket) {
        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, "\n");

        std::istream stream(&buffer);
        std::string message;
        std::getline(stream, message);

        return message;
    }
    void receiveMessage() {
        std::string response_message = read_message(socket_);
        std::cout << "Server> " << response_message << std::endl;
    }

    void closeConnection() {
        std::cout << "\nShutting down the client..." << std::endl;
        socket_.close();
    }

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket socket_;
};

int main(int argc, char* argv[]) {
    // Default values for IP and port
    std::string ip_address = "127.0.0.1";
    unsigned short port = 55000;

    // Parse the port number from command-line arguments, if provided
    if (argc > 1) {
        try {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port provided. Using default port " << port << std::endl;
        }
    }

    try {
        // Create and initialize the client
        Client client(ip_address, port);

        // Send a message to the server
        client.sendMessage("Hello from Client!");

        // Receive a response from the server
        client.receiveMessage();

        // notify about shutdown
        std::cout << "\nShutting down the client..." << std::endl;

        // Close the connection
        client.closeConnection();

    } catch (std::exception& excep) {
        std::cerr << "Exception: " << excep.what() << std::endl;
    }

    return 0;
}