cmake_minimum_required(VERSION 3.10)
project(boost_asio_tcp_async_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS system filesystem log log_setup thread)

# Add the executable
add_executable(async_server async_server.cpp)

# Link Boost libraries
target_link_libraries(async_server PRIVATE Boost::system Boost::filesystem Boost::log Boost::log_setup Boost::thread)

# Compiler definitions
target_compile_definitions(async_server PRIVATE BOOST_ALL_DYN_LINK BOOST_THREAD_USE_DLL)