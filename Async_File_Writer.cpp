#include "Async_File_Writer.h"

// constructor: initializes the file writer and sets the log file path
Async_File_Writer::Async_File_Writer(boost::asio::io_context& context) 
        : io_context(context), strand(context.get_executor()) {
        std::string log_dir = std::filesystem::current_path().string() + "/logs";
        std::filesystem::create_directories(log_dir);
        file_path = log_dir + "/clients_log.txt";
        Logger::get_instance().log_debug("Async File Writer initialized.File path: " + file_path);
    }

// updates the client data and schedules a file write operation if needed
void Async_File_Writer::update_data(const std::map<std::string, unsigned int>& clients) {
    std::set<std::pair<std::string, unsigned int>> new_clients;
    {
        std::lock_guard<std::mutex> lock(snapshot_mutex);

        Logger::get_instance().log_debug("(Async_File_Writer::update_data) : Updating data. Current snapshot size: " + std::to_string(clients_snapshot.size()));

        for (const auto& client : clients) {
        std::pair<std::string, unsigned int> client_entry = {client.first, client.second};

            // Only add clients that are NOT already written
            if (written_clients.find(client_entry) == written_clients.end()) {
                new_clients.insert(client_entry);
                // clients_snapshot.insert(client_entry);
            }
        }
    }

    // post the file-writing operation to the strand
    if (!new_clients.empty()) {
        boost::asio::post(strand, [this, new_clients]() { write_to_file(new_clients); });
        Logger::get_instance().log_info("(Async_File_Writer::update_data()) : File writing operation posted to strand");
    }
}

// writes new client data to the file and marks them as logged
void Async_File_Writer::write_to_file(const std::set<std::pair<std::string, unsigned int>>& new_clients) {
    if (new_clients.empty()) return;

    std::ofstream file(file_path, std::ios::app);  // open file in append mode
    if (file.is_open()) {
        Logger::get_instance().log_info("(Async_File_Writer::write_to_file()) : Writing to file {" + file_path + "}");
        for (const auto& [client_id, client_number] : new_clients) {
            file << "Client #" << client_number << " : [" << client_id << "]\n";
            Logger::get_instance().log_info("(Async_File_Writer::write_to_file()) : Client info {Client #" + std::to_string(client_number) + " : [" + client_id + "]} is written to file " + file_path);
        }

        std::lock_guard<std::mutex> lock(snapshot_mutex);
        written_clients.insert(new_clients.begin(), new_clients.end());  // mark clients as logged
        Logger::get_instance().log_info("(Async_File_Writer::write_to_file()): Mark client as logged");
    } else {
        Logger::get_instance().log_error("(Async_File_Writer::write_to_file()) : Failed to open the file: " + file_path);
    }    
}