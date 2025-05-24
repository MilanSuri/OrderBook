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
    double price;                      // Price associated with this level
    std::deque<Order> orders;         // Queue of orders (FIFO)
    double totalQuantity;             // Total quantity of all orders at this level

    // Initialize with a price and zero total quantity
    explicit PriceLevel(double p) : price(p), totalQuantity(0.0) {}

    // Add an order to the queue and update total quantity
    void addOrder(const Order& order) {
        orders.push_back(order);
        totalQuantity += order.getQuantity();
    }

    // Remove an order by order ID and adjust the total quantity
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
                asks.insert({order.getPrice(), std::move(newLevel)});
            } else {
                i->second.addOrder(order);
            }
        }
    }

    // This function can be defined elsewhere if needed
    void addOrder(const Order & order);
};

// Top-level order book that supports order matching and maintains order history
class OrderBook {
public:
    std::unordered_map<int , Order> orderHistory;

    // Add a new order and attempt to match it
    void addOrder(Order &order, OrderBookSellSide &asks, OrderBookBuySide &bids) {
        // Store the order in history regardless of matching outcome
        orderHistory.insert({order.getOrderId(), order});

        if (order.getSide() == Side::BUY) {
            // Process a BUY order: attempt to match with the lowest available SELLs
            auto it = asks.asks.begin();

            while (it != asks.asks.end() && it->first <= order.getPrice() && order.getQuantity() > 0) {
                PriceLevel& priceLevel = it->second;

                // Match against existing sell orders at this price level (FIFO)
                while (!priceLevel.orders.empty() && order.getQuantity() > 0) {
                    Order &askOrder = priceLevel.orders.front();

                    // Determine the matched quantity
                    double tradeQuantity = std::min(order.getQuantity(), askOrder.getQuantity());

                    // Apply the trade
                    order.reduceQuantity(tradeQuantity);
                    askOrder.reduceQuantity(tradeQuantity);
                    priceLevel.totalQuantity -= tradeQuantity;

                    // Remove fully filled sell order
                    if (askOrder.getQuantity() == 0) {
                        priceLevel.orders.pop_front();
                    }
                }

                // Remove price level if no remaining orders
                if (priceLevel.orders.empty()) {
                    it = asks.asks.erase(it);
                } else {
                    ++it;
                }
            }

            // If buy order is partially filled, add to buy side book
            if (order.getQuantity() > 0) {
                bids.addOrder(order);
            }
        } else {
            // Process a SELL order: attempt to match with the highest available BUYs
            auto it = bids.bids.begin();

            while (it != bids.bids.end() && it->first >= order.getPrice() && order.getQuantity() > 0) {
                PriceLevel& priceLevel = it->second;

                // Match against existing buy orders at this price level (FIFO)
                while (!priceLevel.orders.empty() && order.getQuantity() > 0) {
                    Order &bidOrder = priceLevel.orders.front();

                    // Determine the matched quantity
                    double tradeQuantity = std::min(order.getQuantity(), bidOrder.getQuantity());

                    // Apply the trade
                    order.reduceQuantity(tradeQuantity);
                    bidOrder.reduceQuantity(tradeQuantity);
                    priceLevel.totalQuantity -= tradeQuantity;

                    // Remove fully filled buy order
                    if (bidOrder.getQuantity() == 0) {
                        priceLevel.orders.pop_front();
                    }
                }

                // Remove price level if no remaining orders
                if (priceLevel.orders.empty()) {
                    it = bids.bids.erase(it);
                } else {
                    ++it;
                }
            }

            // If sell order is partially filled, add to sell side book
            if (order.getQuantity() > 0) {
                asks.addAsk(order);
            }
        }
    }
};

#endif //ORDERBOOK_H
