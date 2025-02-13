#include "Logger.h"

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    init_logging();
}

/*
void Logger::init_logging() {
    if (!boost::log::core::get()->get_logging_enabled()) {
        boost::log::add_file_log("app.log");
        boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
        boost::log::add_common_attributes();
        BOOST_LOG_TRIVIAL(info) << "Logging initialized";
    }
}
*/
void Logger::init_logging() {
    // file logging
    boost::log::add_file_log(
        boost::log::keywords::file_name = "server_log_%N.log",
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

    BOOST_LOG_TRIVIAL(info) << "Logging initialized";
}


void Logger::log_info(const std::string& message) {
    BOOST_LOG_TRIVIAL(info) << message;
}

void Logger::log_error(const std::string& message) {
    BOOST_LOG_TRIVIAL(error) << message;
}

void Logger::log_error(const std::exception& e) {
    BOOST_LOG_TRIVIAL(error) << "Exception: " << e.what();
}

void Logger::log_debug(const std::string& message) {
    BOOST_LOG_TRIVIAL(debug) << message;
}


    

   


