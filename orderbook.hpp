//
// Created by Milan Suri on 5/9/25.
//

#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <queue>

enum class Side {
    BUY,
    SELL
};

class Order {
private:
    int orderId;
    double price;
    double quantity;
    Side side;
public:


    Order(int new_orderId, double new_price, double new_quantity, Side new_side) {
        orderId = new_orderId;
        price = new_price;
        quantity = new_quantity;
        side = new_side;
    }
    [[nodiscard]] int getOrderId() const { return orderId; }
    [[nodiscard]] double getPrice() const { return price; }
    [[nodiscard]] double getQuantity() const { return quantity; }
    Side getSide() { return side; }
};



class PriceLevel {
public:
    double price;;
    std::deque<Order> orders;
    int totalQuantity;

    void addOrder(Order &order) {
        orders.push_back(order);
        totalQuantity++;
    }

    void removeOrder(Order &order, Order &orderId) {
        if (orders.empty()) {
            std::cout << "No orders to remove" << std::endl;
        } else {
            auto it = std::find(orders.begin(), orders.end(), orderId);
            if (it != orders.end()) {
                orders.erase(it);
                totalQuantity--;
            }
        }
    }

    [[nodiscard]] int getTotalQuantity() const { return totalQuantity; }

    bool getFirstOrder(Order &order) {
        if (orders.empty()) {
            std::cout << "No orders";
            return false;
        }
        order = orders.front();
        return true;
    }
};

class OrderBookBuySide {
public:
    std::map<double, PriceLevel, std::greater<>> bids;
    void addOrder(Order &order) {
        if (order.getSide() == Side::BUY) {

        }
    }
};

class OrderBookSellSide {
public:
    std::map<double, PriceLevel, std::lower<double>> asks;

};

class OrderBook {
public:
    std::unordered_map<int , Order> orderHistory;
};


#endif //ORDERBOOK_H
