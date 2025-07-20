#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <deque>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <sqlite3.h>
#include <iomanip>
#include <string>

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
    std::string ticker;
public:
    // Constructor to initialize order fields
    Order(int new_orderId, double new_price, double new_quantity, Side new_side, std::string new_ticker) {
        orderId = new_orderId;
        price = new_price;
        quantity = new_quantity;
        side = new_side;
        ticker = new_ticker;
    }

    // Accessors (getters)
    [[nodiscard]] int getOrderId() const { return orderId; }
    [[nodiscard]] double getPrice() const { return price; }
    [[nodiscard]] double getQuantity() const { return quantity; }
    Side getSide() const { return side; }
    [[nodiscard]] std::string getTicker() const { return ticker; }

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

    std::unordered_map<std::string, OrderBookSellSide> sellSides;
    std::unordered_map<std::string, OrderBookBuySide> buySides;

    sqlite3 *DB;

    void initializeDB() {
        int exit = sqlite3_open("orderhistory.db", &DB);

        // Create the table only if it doesn't already exist
        std::string sql = "CREATE TABLE IF NOT EXISTS ORDERS("
                          "ORDER_ID INT PRIMARY KEY NOT NULL,"
                          "TICKER TEXT NOT NULL,"
                          "PRICE REAL NOT NULL,"
                          "QUANTITY REAL NOT NULL,"
                          "SIDE INT NOT NULL);";

        char* messageError;
        exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messageError);
        if (exit != SQLITE_OK) {
            std::cerr << "Error creating table: " << messageError << std::endl;
            sqlite3_free(messageError);
        }
    }



    // Add a new order and attempt to match it
    void addOrder(Order &order) {

        std::string ticker = order.getTicker();
        OrderBookSellSide& asks = sellSides[ticker];
        OrderBookBuySide& bids = buySides[ticker];

        // Store the order in history regardless of matching outcome
        std::string sql("INSERT INTO ORDERS (ORDER_ID, TICKER, PRICE, QUANTITY, SIDE) VALUES (?, ?, ?, ?, ?);");
        sqlite3_stmt *statement;
        sqlite3_prepare_v2(DB, sql.c_str(), -1, &statement, nullptr);

        sqlite3_bind_int(statement, 1, order.getOrderId());
        sqlite3_bind_text(statement, 2, order.getTicker().c_str(), -1, nullptr);
        sqlite3_bind_double(statement, 3, order.getPrice());
        sqlite3_bind_double(statement, 4, order.getQuantity());
        sqlite3_bind_int(statement, 5, static_cast<int>(order.getSide()));
        sqlite3_step(statement);
        sqlite3_finalize(statement);

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

    void loadsOrdersFromDB() {
        std::string sql = "SELECT * FROM ORDERS";
        sqlite3_stmt* statement;

        int rc = sqlite3_prepare_v2(DB, sql.c_str(), -1, &statement, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(DB) << std::endl;
            return;
        }

        while (sqlite3_step(statement) == SQLITE_ROW) {
            int orderId = sqlite3_column_int(statement, 0);
            const unsigned char* raw = sqlite3_column_text(statement, 1);
            std::string ticker = raw ? reinterpret_cast<const char*>(raw) : "";
            double price = sqlite3_column_double(statement, 2);
            double quantity = sqlite3_column_double(statement, 3);
            int sideInt = sqlite3_column_int(statement, 4);
            Side side = (sideInt == 0) ? Side::BUY : Side::SELL;

            Order order(orderId, price, quantity, side, ticker);

            if (side == Side::BUY) {
                buySides[ticker].addOrder(order);
            } else {
                sellSides[ticker].addAsk(order);
            }
        }

        sqlite3_finalize(statement);
    }


    // Remove an order from both order book and history by its ID
    bool removeOrderById(int orderId) {
    std::string sql = "DELETE FROM ORDERS WHERE ORDER_ID = ?;";
    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(DB, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare delete statement: " << sqlite3_errmsg(DB) << std::endl;
        return false;
    }

    sqlite3_bind_int(statement, 1, orderId);

    int result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result == SQLITE_DONE) {
        // Check if any row was actually deleted
        if (sqlite3_changes(DB) > 0) {
            return true;
        } else {
            std::cout << "No order found with ID " << orderId << ". Nothing to remove.\n";
            return false;
        }
    } else {
        std::cerr << "Error deleting order: " << sqlite3_errmsg(DB) << std::endl;
        return false;
    }
}
    void displayOrders() const {
    const char* sql = "SELECT ORDER_ID, TICKER, PRICE, QUANTITY, SIDE FROM ORDERS;";
    sqlite3_stmt* statement;

    int rc = sqlite3_prepare_v2(DB, sql, -1, &statement, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(DB) << std::endl;
        return;
    }

    // Print headers
    std::cout << std::left
              << std::setw(10) << "ORDER_ID"
              << std::setw(12) << "TICKER"
              << std::setw(12) << "PRICE"
              << std::setw(12) << "QUANTITY"
              << std::setw(8) << "SIDE" << std::endl;

    std::cout << std::string(54, '-') << std::endl;

    bool hasRows = false;

    while (sqlite3_step(statement) == SQLITE_ROW) {
        hasRows = true;
        int orderId = sqlite3_column_int(statement, 0);
        const unsigned char* rawTicker = sqlite3_column_text(statement, 1);
        std::string ticker = rawTicker ? reinterpret_cast<const char*>(rawTicker) : "";
        double price = sqlite3_column_double(statement, 2);
        double quantity = sqlite3_column_double(statement, 3);
        int sideInt = sqlite3_column_int(statement, 4);
        std::string sideStr = (sideInt == 0) ? "BUY" : "SELL";

        std::cout << std::left
                  << std::setw(10) << orderId
                  << std::setw(12) << ticker
                  << std::setw(12) << price
                  << std::setw(12) << quantity
                  << std::setw(8) << sideStr << std::endl;
    }

    sqlite3_finalize(statement);

    if (!hasRows) {
        std::cout << "No orders stored yet.\n";
    }
}

    void displayActiveTickers(const std::string& user_ticker) const {
        std::cout << "\nActive Orders for Ticker: " << user_ticker << "\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << std::left
                  << std::setw(10) << "ORDER_ID"
                  << std::setw(12) << "PRICE"
                  << std::setw(12) << "QUANTITY"
                  << std::setw(10) << "SIDE" << "\n";
        std::cout << std::string(60, '-') << "\n";

        bool foundOrders = false;

        // Display BUY side orders
        auto buyIt = buySides.find(user_ticker);
        if (buyIt != buySides.end()) {
            for (const auto& [price, level] : buyIt->second.bids) {
                for (const Order& order : level.orders) {
                    std::cout << std::left
                              << std::setw(10) << order.getOrderId()
                              << std::setw(12) << order.getPrice()
                              << std::setw(12) << order.getQuantity()
                              << std::setw(10) << "BUY" << "\n";
                    foundOrders = true;
                }
            }
        }

        // Display SELL side orders
        auto sellIt = sellSides.find(user_ticker);
        if (sellIt != sellSides.end()) {
            for (const auto& [price, level] : sellIt->second.asks) {
                for (const Order& order : level.orders) {
                    std::cout << std::left
                              << std::setw(10) << order.getOrderId()
                              << std::setw(12) << order.getPrice()
                              << std::setw(12) << order.getQuantity()
                              << std::setw(10) << "SELL" << "\n";
                    foundOrders = true;
                }
            }
        }

        if (!foundOrders) {
            std::cout << "No active orders found for ticker " << user_ticker << ".\n";
        };
    }
};


#endif // ORDERBOOK_H