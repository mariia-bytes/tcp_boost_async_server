#include <thread>
#include "Server.h"


// global variables for default configurations
unsigned short port = 55000; // default port
std::string ip_address = "0.0.0.0"; // default IP address


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

        // determine the number of threads for the thread pool
        unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> thread_pool;
        
        // create and start the server
        Server server(io_context, ip_address, port);

        // handle termination signals for graceful shutdown
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const boost::system::error_code&, int) {
            std::cout << "\n\nShutting down the server..." << std::endl;
            Logger::get_instance().log_info("Shutting down the server...");
            io_context.stop();
        });

        // launch worker threads to run the io_context event loop
        for (unsigned int i = 0; i < num_threads; i++) {
            thread_pool.emplace_back([&io_context]() { io_context.run(); });
        }

        // wait for all threads to complete execution before exiting
        for (auto& thread : thread_pool) {
            thread.join();
        }
    
    } catch (std::exception& e) {
        std::cerr << "\nSomething went wrong. See server logs for more details\n";
        Logger::get_instance().log_error(e);
    }

    return 0;
}