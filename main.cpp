#include <iostream>
#include "orderbook.hpp"

int main() {
    int decision;
    bool session = true;
    int orderIdCounter = 1;

    std::cout << "Welcome to the order book system \n";

    OrderBook orderBook;
    OrderBookBuySide buySide;
    OrderBookSellSide sellSide;

    while (session == true){
        std::cout << "Press [0] to leave the order book \n";
        std::cout << "Press [1] to add a bid \n";
        std::cout << "Press [2] to add an ask \n";
        std::cout << "Press [3] to remove a bid \n";
        std::cout << "Press [4] to remove an ask \n";
        std::cout << "Press [5] to show order history \n";
        std::cout << "Press [6] to show all orders \n";

        std::cin >> decision;

        switch (decision) {
            case 0: {
                session = false;
                std::cout << "Thanks for using the order book system \n";
                break;
            } case 1: {
                double price, quantity;
                std::cout << "Enter the price of the bid: ";
                std::cin >> price;
                std::cout << "Enter the quantity: ";
                std::cin >> quantity;
                Order order (orderIdCounter++, price, quantity, Side::BUY);
                orderBook.addOrder(order, sellSide, buySide);
                break;
            } case 2: {
                double price, quantity;
                std::cout << "Enter the price of the ask: ";
                std::cin >> price;
                std::cout << "Enter the quantity: ";
                std::cin >> quantity;
                Order order (orderIdCounter++, price, quantity, Side::SELL);
                orderBook.addOrder(order, sellSide, buySide);
                break;
            } case 3: {
                int orderId;
                std::cout << "Enter the order id: ";
                std::cin >> orderId;
                for (auto & [price, level] : buySide.bids ) {
                    level.removeOrder(orderId);
                }
                break;
            } case 4: {
                int orderId;
                std::cout << "Enter the order id: ";
                std::cin >> orderId;
                for (auto & [price, level] : sellSide.asks) {
                    level.removeOrder(orderId);
                }
                break;
            } case 5: {
                std::cout << "Order History:\n";
                for (const auto& [id, order] : orderBook.orderHistory) {
                    std::cout << "ID: " << order.getOrderId()
                              << " | Price: " << order.getPrice()
                              << " | Qty: " << order.getQuantity()
                              << " | Side: " << (order.getSide() == Side::BUY ? "BUY" : "SELL") << '\n';
                }
                break;
            } case 6: {
                std::cout << "Active Buy Orders:\n";
                for (const auto& [price, level] : buySide.bids) {
                    std::cout << "Price: " << price << " | Total Qty: " << level.getTotalQuantity() << '\n';
                    for (const auto& order : level.orders) {
                        std::cout << "   ID: " << order.getOrderId() << " | Qty: " << order.getQuantity() << '\n';
                    }
                }

                std::cout << "Active Sell Orders:\n";
                for (const auto& [price, level] : sellSide.asks) {
                    std::cout << "Price: " << price << " | Total Qty: " << level.getTotalQuantity() << '\n';
                    for (const auto& order : level.orders) {
                        std::cout << "   ID: " << order.getOrderId() << " | Qty: " << order.getQuantity() << '\n';
                    }
                }
                break;
            } default: {
                std::cout << "Invalid Choice\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
    }
    return 0;
}
