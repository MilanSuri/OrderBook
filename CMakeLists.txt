cmake_minimum_required(VERSION 3.31)
project(OrderBook)

set(CMAKE_CXX_STANDARD 20)

# Find libpqxx using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Your executable
add_executable(OrderBook
        main.cpp
        OrderBook.hpp
        # Add .cpp files here if needed
)

# Apply pkg-config include/link flags
target_include_directories(OrderBook PRIVATE ${PQXX_INCLUDE_DIRS})
target_link_directories(OrderBook PRIVATE ${PQXX_LIBRARY_DIRS})
target_link_libraries(OrderBook PRIVATE ${PQXX_LIBRARIES})
target_link_libraries(OrderBook PRIVATE sqlite3)