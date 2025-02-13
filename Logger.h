#ifndef LOGGER_H
#define LOGGER_H 

#include <string>
#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>

class Logger {
public:
    static Logger& get_instance();

    void log_info(const std::string& message);
    void log_error(const std::string& message);
    void log_error(const std::exception& e);
    void log_debug(const std::string& message);

private:
    Logger();
    ~Logger() = default;

    void init_logging();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

};

#endif // LOGGER_H

