//
// Created by Milan Suri on 6/3/25.
//

#include "orderbook.hpp"

// Order class implementations
Order::Order(int new_orderId, double new_price, double new_quantity, Side new_side)
    : orderId(new_orderId), price(new_price), quantity(new_quantity), side(new_side) {}

int Order::getOrderId() const { return orderId; }
double Order::getPrice() const { return price; }
double Order::getQuantity() const { return quantity; }
Side Order::getSide() const { return side; }

void Order::reduceQuantity(double amount) {
    if (amount > 0) {
        quantity -= amount;
        if (quantity < 0) quantity = 0;
    }
}

// PriceLevel implementations
PriceLevel::PriceLevel(double p) : price(p), totalQuantity(0.0) {}

void PriceLevel::addOrder(const Order& order) {
    orders.push_back(order);
    totalQuantity += order.getQuantity();
}

void PriceLevel::removeOrder(int orderId) {
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

double PriceLevel::getTotalQuantity() const {
    return totalQuantity;
}

bool PriceLevel::getFirstOrder(Order& order) const {
    if (orders.empty()) {
        std::cout << "No orders" << std::endl;
        return false;
    }
    order = orders.front();
    return true;
}

bool PriceLevel::isEmpty() const {
    return orders.empty();
}

// OrderBookBuySide implementations
void OrderBookBuySide::addOrder(Order &order) {
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

// OrderBookSellSide implementations
void OrderBookSellSide::addAsk(Order &order) {
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

// OrderBook implementations
void OrderBook::addOrder(Order &order, OrderBookSellSide &asks, OrderBookBuySide &bids) {
    orderHistory.insert({order.getOrderId(), order});

    if (order.getSide() == Side::BUY) {
        auto it = asks.asks.begin();

        while (it != asks.asks.end() && it->first <= order.getPrice() && order.getQuantity() > 0) {
            PriceLevel& priceLevel = it->second;

            while (!priceLevel.orders.empty() && order.getQuantity() > 0) {
                Order &askOrder = priceLevel.orders.front();

                double tradeQuantity = std::min(order.getQuantity(), askOrder.getQuantity());

                std::cout << "Trade executed: " << tradeQuantity << " @ " << priceLevel.price << " (BUY matched with SELL)\n";
                order.reduceQuantity(tradeQuantity);
                askOrder.reduceQuantity(tradeQuantity);
                priceLevel.totalQuantity -= tradeQuantity;

                if (askOrder.getQuantity() == 0) {
                    priceLevel.orders.pop_front();
                }
            }

            if (priceLevel.orders.empty()) {
                it = asks.asks.erase(it);
            } else {
                ++it;
            }
        }

        if (order.getQuantity() > 0) {
            bids.addOrder(order);
        }
    } else {
        auto it = bids.bids.begin();

        while (it != bids.bids.end() && it->first >= order.getPrice() && order.getQuantity() > 0) {
            PriceLevel& priceLevel = it->second;

            while (!priceLevel.orders.empty() && order.getQuantity() > 0) {
                Order &bidOrder = priceLevel.orders.front();

                double tradeQuantity = std::min(order.getQuantity(), bidOrder.getQuantity());

                std::cout << "Trade executed: " << tradeQuantity << " @ " << priceLevel.price << " (SELL matched with BUY)\n";
                order.reduceQuantity(tradeQuantity);
                bidOrder.reduceQuantity(tradeQuantity);
                priceLevel.totalQuantity -= tradeQuantity;

                if (bidOrder.getQuantity() == 0) {
                    priceLevel.orders.pop_front();
                }
            }

            if (priceLevel.orders.empty()) {
                it = bids.bids.erase(it);
            } else {
                ++it;
            }
        }

        if (order.getQuantity() > 0) {
            asks.addAsk(order);
        }
    }
}

bool OrderBook::removeOrderById(int orderId, OrderBookBuySide& buySide, OrderBookSellSide& sellSide) {
    const auto i = orderHistory.find(orderId);
    if (i == orderHistory.end()) {
        std::cout << "Order not found in history.\n";
        return false;
    }

    Order order = i->second;
    double price = order.getPrice();

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

    orderHistory.erase(i);
    return true;
}
