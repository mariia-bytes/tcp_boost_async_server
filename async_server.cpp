#include <thread>
#include "Server.h"


// global variables for default
unsigned short port = 55000; // default port
std::string ip_address = "127.0.0.1"; // default IP address


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
        
        // path to save the clients log
        std::string file_path = std::filesystem::current_path().string() + "/logs/clients_log.txt";
        
        // create and start the server
        Server server(io_context, ip_address, port, file_path);

        // handling signal for graceful shutdown
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const boost::system::error_code&, int) {
            std::cout << "\n\nShutting down the server..." << std::endl;
            Logger::get_instance().log_info("Shutting down the server...");
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
        std::cerr << "\nSomething went wrong. See server logs for more details\n";
        Logger::get_instance().log_error(e);
    }

    return 0;
}