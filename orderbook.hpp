#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP

#include <map>
#include <unordered_map>
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
    Order(int new_orderId, double new_price, double new_quantity, Side new_side);

    [[nodiscard]] int getOrderId() const;
    [[nodiscard]] double getPrice() const;
    [[nodiscard]] double getQuantity() const;
    Side getSide() const;

    void reduceQuantity(double amount);
};

// Represents a single price level in the order book
class PriceLevel {
public:
    double price;
    std::deque<Order> orders;
    double totalQuantity;

    explicit PriceLevel(double p);

    void addOrder(const Order& order);
    void removeOrder(int orderId);
    [[nodiscard]] double getTotalQuantity() const;
    bool getFirstOrder(Order& order) const;
    bool isEmpty() const;
};

// Represents the buy side of the order book (bids)
class OrderBookBuySide {
public:
    std::map<double, PriceLevel, std::greater<>> bids;

    void addOrder(Order &order);
};

// Represents the sell side of the order book (asks)
class OrderBookSellSide {
public:
    std::map<double, PriceLevel, std::less<>> asks;

    void addAsk(Order &order);
};

// Top-level order book that supports order matching and maintains order history
class OrderBook {
public:
    std::unordered_map<int , Order> orderHistory;

    void addOrder(Order &order, OrderBookSellSide &asks, OrderBookBuySide &bids);
    bool removeOrderById(int orderId, OrderBookBuySide& buySide, OrderBookSellSide& sellSide);
};

#endif // ORDERBOOK_HPP
