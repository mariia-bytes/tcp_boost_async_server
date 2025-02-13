#ifndef ASYNC_FILE_WRITER_H
#define ASYNC_FILE_WRITER_H

#include <boost/asio.hpp>
#include <set>
#include <map>
#include <mutex>
#include <fstream>
#include "Logger.h"

class Async_File_Writer {
private:
    boost::asio::io_context& io_context;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    std::string file_path;
    std::set<std::pair<std::string, unsigned int>> clients_snapshot;
    std::set<std::pair<std::string, unsigned int>> written_clients;
    std::mutex snapshot_mutex; // protects clients snapshot    

public:
    Async_File_Writer(boost::asio::io_context& context, const std::string& path);
    ~Async_File_Writer() = default;

    void update_data(const std::map<std::string, unsigned int>& clients);

private:
    void write_to_file(const std::set<std::pair<std::string, unsigned int>>& new_clients);
};

#endif // ASYNC_FILE_WRITER_H