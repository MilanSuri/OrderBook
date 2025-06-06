#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <deque>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>


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
    Side getSide() const { return side; }

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

    // Retrieve the total quantity at this price level
    [[nodiscard]] double getTotalQuantity() const { return totalQuantity; }

    // Peek at the first order in the queue
    bool getFirstOrder(Order& order) const {
        if (orders.empty()) {
            std::cout << "No orders" << std::endl;
            return false;
        }
        order = orders.front();
        return true;
    }

    // Check if this price level contains any orders
    bool isEmpty() const {
        return orders.empty();
    }
};

// Represents the buy side of the order book (bids)
// Orders are sorted by price in descending order (highest first)
class OrderBookBuySide {
public:
    std::map<double, PriceLevel, std::greater<>> bids;

    // Add a buy order to the corresponding price level
    void addOrder(Order &order) {
        if (order.getSide() == Side::BUY) {
            auto i = bids.find(order.getPrice());
            if (i == bids.end()) {
                PriceLevel newLevel(order.getPrice());
                newLevel.addOrder(order);
                bids.insert({order.getPrice(), std::move(newLevel)});
            } else {
                i->second.addOrder(order);
            }
        }
    }
};

// Represents the sell side of the order book (asks)
// Orders are sorted by price in ascending order (lowest first)
class OrderBookSellSide {
public:
    std::map<double, PriceLevel, std::less<>> asks;

    // Add a sell order to the appropriate price level
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

    // Optional: declared but not defined here
    void addOrder(const Order & order);
};

// Top-level order book that supports order matching and maintains order history
class OrderBook {
public:
    std::unordered_map<int , Order> orderHistory; // Track all orders by ID

    // Add a new order and attempt to match it
    void addOrder(Order &order, OrderBookSellSide &asks, OrderBookBuySide &bids) {
        // Store the order in history regardless of matching outcome
        orderHistory.insert({order.getOrderId(), order});

        if (order.getSide() == Side::BUY) {
            // Match BUY order against SELL orders
            auto it = asks.asks.begin();

            while (it != asks.asks.end() && it->first <= order.getPrice() && order.getQuantity() > 0) {
                PriceLevel& priceLevel = it->second;

                // Match against existing SELL orders at this price level (FIFO)
                while (!priceLevel.orders.empty() && order.getQuantity() > 0) {
                    Order &askOrder = priceLevel.orders.front();

                    // Determine how much can be traded
                    double tradeQuantity = std::min(order.getQuantity(), askOrder.getQuantity());

                    // Apply the trade
                    std::cout << "Trade executed: " << tradeQuantity << " @ " << priceLevel.price << " (BUY matched with SELL)\n";
                    order.reduceQuantity(tradeQuantity);
                    askOrder.reduceQuantity(tradeQuantity);
                    priceLevel.totalQuantity -= tradeQuantity;

                    // Remove fully filled order
                    if (askOrder.getQuantity() == 0) {
                        priceLevel.orders.pop_front();
                    }
                }

                // Clean up empty price level
                if (priceLevel.orders.empty()) {
                    it = asks.asks.erase(it);
                } else {
                    ++it;
                }
            }

            // If unfilled quantity remains, add to BUY side book
            if (order.getQuantity() > 0) {
                bids.addOrder(order);
            }
        } else {
            // Match SELL order against BUY orders
            auto it = bids.bids.begin();

            while (it != bids.bids.end() && it->first >= order.getPrice() && order.getQuantity() > 0) {
                PriceLevel& priceLevel = it->second;

                // Match against existing BUY orders at this price level (FIFO)
                while (!priceLevel.orders.empty() && order.getQuantity() > 0) {
                    Order &bidOrder = priceLevel.orders.front();

                    // Determine how much can be traded
                    double tradeQuantity = std::min(order.getQuantity(), bidOrder.getQuantity());

                    // Apply the trade
                    std::cout << "\n Trade executed: " << tradeQuantity << " @ " << priceLevel.price << " (SELL matched with BUY)" << std::endl;
                    order.reduceQuantity(tradeQuantity);
                    bidOrder.reduceQuantity(tradeQuantity);
                    priceLevel.totalQuantity -= tradeQuantity;

                    // Remove fully filled order
                    if (bidOrder.getQuantity() == 0) {
                        priceLevel.orders.pop_front();
                    }
                }

                // Clean up empty price level
                if (priceLevel.orders.empty()) {
                    it = bids.bids.erase(it);
                } else {
                    ++it;
                }
            }

            // If unfilled quantity remains, add to SELL side book
            if (order.getQuantity() > 0) {
                asks.addAsk(order);
            }
        }
    }

    // Remove an order from both order book and history by its ID
    bool removeOrderById(int orderId, OrderBookBuySide& buySide, OrderBookSellSide& sellSide) {
        const auto i = orderHistory.find(orderId);
        if (i == orderHistory.end()) {
            std::cout << "Order not found in history.\n";
            return false;
        }

        Order order = i->second;
        double price = order.getPrice();

        // Remove from the correct side (BUY or SELL)
        if (order.getSide() == Side::BUY) {
            auto levelIt = buySide.bids.find(price);
            if (levelIt != buySide.bids.end()) {
                levelIt->second.removeOrder(orderId);
                if (levelIt->second.isEmpty()) {
                    buySide.bids.erase(levelIt);
                }
            }
        } else {
            auto levelIt = sellSide.asks.find(price);
            if (levelIt != sellSide.asks.end()) {
                levelIt->second.removeOrder(orderId);
                if (levelIt->second.isEmpty()) {
                    sellSide.asks.erase(levelIt);
                }
            }
        }

        // Remove from history
        orderHistory.erase(i);
        return true;
    }
};

#endif // ORDERBOOK_H