//
// Created by Milan Suri on 5/9/25.
//

#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <queue>
#include <deque>
#include <iostream>
#include <algorithm>

// Enumeration to represent order side (buy or sell)
enum class Side {
    BUY,
    SELL
};

// Class representing a single order
class Order {
private:
    int orderId;
    double price;
    double quantity;
    Side side;
public:
    // Constructor to initialize order fields
    Order(int new_orderId, double new_price, double new_quantity, Side new_side) {
        orderId = new_orderId;
        price = new_price;
        quantity = new_quantity;
        side = new_side;
    }

    // Accessors (getters)
    [[nodiscard]] int getOrderId() const { return orderId; }
    [[nodiscard]] double getPrice() const { return price; }
    [[nodiscard]] double getQuantity() const { return quantity; }
    Side getSide() { return side; }
};

    // Reduce the order quantity by a given amount, ensuring non-negative result
    void reduceQuantity(double amount) {
        if (amount > 0) {
            quantity -= amount;
            if (quantity < 0) quantity = 0;
        }
    }
};

// Represents a single price level in the order book
class PriceLevel {
public:
    double price;
    std::deque<Order> orders;
    double totalQuantity;

    // Constructor to initialize price and total quantity
    explicit PriceLevel(double p) : price(p), totalQuantity(0.0) {}

    // Add order and sum actual quantity
    void addOrder(const Order& order) {
        orders.push_back(order);
        totalQuantity += order.getQuantity();
    }

    //  Remove by orderId, adjust totalQuantity accordingly
    void removeOrder(int orderId) {
        if (orders.empty()) {
            std::cout << "No orders to remove" << std::endl;
            return;
        }

        auto it = std::find_if(orders.begin(), orders.end(),
            [orderId](const Order& o) { return o.getOrderId() == orderId; });

        if (it != orders.end()) {
            totalQuantity -= it->getQuantity();
            orders.erase(it);
        } else {
            std::cout << "Order not found" << std::endl;
        }
    }

    [[nodiscard]] double getTotalQuantity() const { return totalQuantity; }

    // Peek at the front order (FIFO)
    bool getFirstOrder(Order& order) const {
        if (orders.empty()) {
            std::cout << "No orders" << std::endl;
            return false;
        }
        order = orders.front();
        return true;
    }

    // Check if price level is empty
    bool isEmpty() const {
        return orders.empty();
    }
};

class OrderBookBuySide {
public:
    std::map<double, PriceLevel, std::greater<>> bids;
    void addOrder(Order &order) {
        if (order.getSide() == Side::BUY) {
            auto i = bids.find(order.getPrice());
            if (i == bids.end()) {
                PriceLevel newLevel(order.getPrice());
                newLevel.addOrder(order);
                bids.insert({order.getPrice(), std::move (newLevel)});
            } else {
                i->second.addOrder(order);
            }
        }
    }
};

class OrderBookSellSide {
public:
    std::map<double, PriceLevel, std::less<>> asks;
    void addAsk(Order &order) {
        if (order.getSide() == Side::SELL) {
            auto i = asks.find(order.getPrice());
            if (i == asks.end()) {
                PriceLevel newLevel(order.getPrice());
                newLevel.addOrder(order);
                asks.insert({order.getPrice(), std::move (newLevel)});
            } else {
                i->second.addOrder(order);
            }
        }
    }

    void addOrder(const Order & order);
};

class OrderBook {
public:
    std::unordered_map<int , Order> orderHistory;
};


#endif //ORDERBOOK_H
