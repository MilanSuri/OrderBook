cmake_minimum_required(VERSION 3.31)
project(OrderBook)

set(CMAKE_CXX_STANDARD 20)

# Use pkg-config to find libpqxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Find required libraries
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)  # From Homebrew

# Your executable
add_executable(OrderBook
        main.cpp
        OrderBook.hpp
        # Add other .cpp/.hpp files as needed
)

# Include directories
target_include_directories(OrderBook PRIVATE
        ${PQXX_INCLUDE_DIRS}
)

# Link directories (for libpqxx, usually not needed on macOS with pkg-config)
target_link_directories(OrderBook PRIVATE ${PQXX_LIBRARY_DIRS})

# Link all required libraries
target_link_libraries(OrderBook PRIVATE
        ${PQXX_LIBRARIES}
        sqlite3
        ${CURL_LIBRARIES}
        nlohmann_json::nlohmann_json
)
