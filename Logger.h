#ifndef LOGGER_H
#define LOGGER_H 

#include <string>
#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <filesystem>

/**
 * @class Logger
 * @brief Provides a centralized logging mechanism for the application.
 *
 * The Logger class is a singleton that facilitates structured logging for debugging,
 * error tracking, and information reporting. It uses Boost.Log to write log messages 
 * to both the console and log files, with support for different severity levels 
 * (info, debug, and error). Logs are stored in a designated directory, and file 
 * rotation is handled automatically to manage log size efficiently.
 *
 * @note This class ensures thread-safe logging and is intended for use in 
 *       multi-threaded networked applications.
 */
class Logger {
public:
    /**
     * @brief Retrieves the singleton instance of the Logger.
     * @return Reference to the Logger instance.
     */
    static Logger& get_instance();

    /**
     * @brief Logs an info message.
     * @param message The message to be logged.
     */
    void log_info(const std::string& message);

    /**
     * @brief Logs an error message.
     * @param message The error message to be logged.
     */
    void log_error(const std::string& message);

    /**
     * @brief Logs an exception message.
     * @param e The exception object whose message will be logged.
     */
    void log_error(const std::exception& e);

    /**
     * @brief Logs a debug message.
     * @param message The debug message to be logged.
     */
    void log_debug(const std::string& message);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     */
    Logger();
    ~Logger() = default;

    /**
     * @brief Initializes the logging system, setting up file logging and attributes.
     */
    void init_logging();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

};

#endif // LOGGER_H

