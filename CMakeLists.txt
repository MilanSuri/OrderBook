cmake_minimum_required(VERSION 3.31)
project(OrderBook)

set(CMAKE_CXX_STANDARD 20)

add_executable(OrderBook main.cpp
        OrderBook.hpp
        orderbook.cpp)
