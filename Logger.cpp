#include "Logger.h"

// retrieves the singleton instance of the Logger
Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

// constructor: Initializes the logging system
Logger::Logger() {
    init_logging();
}

// sets up file logging and configures logging attributes
void Logger::init_logging() {
    // define log dir
    std::string log_dir = std::filesystem::current_path().string() + "/logs";
    std::filesystem::create_directories(log_dir);

    // defibe log file path
    std::string log_file = log_dir + "/server_log_%N.log";

    // file logging
    boost::log::add_file_log(
        boost::log::keywords::file_name = log_file,
        boost::log::keywords::rotation_size = 10 * 1024 * 1024, // max 10 MB per file
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::format = "[%TimeStamp%] [%Severity%]: %Message%"
    );

    // common attributes like timestamp, thread ID
    boost::log::add_common_attributes();

    // log filter level
    boost::log::core::get()->set_filter(
        boost::log::trivial::severity >= boost::log::trivial::debug
    );

    BOOST_LOG_TRIVIAL(debug) << "Logging initialized";
}

// logs an info message
void Logger::log_info(const std::string& message) {
    BOOST_LOG_TRIVIAL(info) << message;
}

// logs an error message
void Logger::log_error(const std::string& message) {
    BOOST_LOG_TRIVIAL(error) << message;
}

// logs an exception message, extracting its description
void Logger::log_error(const std::exception& e) {
    BOOST_LOG_TRIVIAL(error) << "Exception: " << e.what();
}

// logs a debug message
void Logger::log_debug(const std::string& message) {
    BOOST_LOG_TRIVIAL(debug) << message;
}