#include "Async_File_Writer.h"

Async_File_Writer::Async_File_Writer(boost::asio::io_context& context, const std::string& path) 
        : io_context(context), strand(context.get_executor()), file_path(path) {
        std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
        Logger::get_instance().log_debug("Async File Writer initialized.File path: " + file_path);
    }


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