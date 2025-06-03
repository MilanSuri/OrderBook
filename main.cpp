#include <iostream>
#include "orderbook.hpp"
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <limits>


OrderBook orderBook;
OrderBookBuySide buySide;
OrderBookSellSide sellSide;
int orderIdCounter = 1;



enum class EventType {
    ADDBID,
    ADDASK,
    REMOVEORDER,
    ORDERHISTORY,
    DISPLAYORDERS,
    UNKNOWN,
    QUIT
};

struct Event {
    EventType type;
    explicit Event(EventType t) : type(t) {

    }
};

class EventDispatcher {
    using Handler = std::function<void(const Event)>;
    std::unordered_map<EventType, Handler> handlers;
    public:

    void registerHandler(EventType type, Handler handler) {
        handlers[type] = handler;
    }

    void dispatch(Event event) {
        auto it = handlers.find(event.type);
        if (it != handlers.end()) {
            it->second(event);
        } else {
            std::cout << "No handler for this event \n";
        }
    }
};

std::string toLower (std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
EventType parseInput(const std::string& input) {
    std::string cmd = toLower(input);
    if (cmd == "add bid") return EventType::ADDBID;
    if (cmd == "add ask") return EventType::ADDASK;
    if (cmd == "remove order") return EventType::REMOVEORDER;
    if (cmd == "order history") return EventType::ORDERHISTORY;
    if (cmd == "display orders") return EventType::DISPLAYORDERS;
    if (cmd == "quit") return EventType::QUIT;
    return EventType::UNKNOWN;
}

void removeHandler(const Event& event) {
    std::cout << "Enter order ID to remove: ";
    int orderId;
    std::cin >> orderId;
    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid order ID. Try again.\n";
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    bool removed = orderBook.removeOrderById(orderId, buySide, sellSide);

    if (removed) {
        std::cout << "Order " << orderId << " removed successfully.\n";
    } else {
        std::cout << "Failed to remove order " << orderId << ".\n";
    }
}

int main() {
    EventDispatcher dispatcher;

    // Register event handlers
    dispatcher.registerHandler(EventType::ADDBID, [](const Event&) {
        std::cout << "Adding bid\n";
        double price, quantity;
        std::cout << "Enter the price of the bid: ";
        std::cin >> price;
        if (std::cin.fail()) {
            std::cin.clear(); // Clear error flag
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard input
            std::cout << "Invalid price. Try again.\n";
            return; // Exit the handler without adding the order
            }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Enter the quantity: ";
        std::cin >> quantity;
         if (std::cin.fail()) {
             std::cin.clear();
             std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
             std::cout << "Invalid quantity. Try again.\n";
             return;
    }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        Order order (orderIdCounter++, price, quantity, Side::BUY);
        orderBook.addOrder(order, sellSide, buySide);
    });

    dispatcher.registerHandler(EventType::ADDASK, [](const Event&) {
        std::cout << "Adding ask\n";
        double price, quantity;
        std::cout << "Enter the price of the ask: ";
        std::cin >> price;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid price. Try again.\n";
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Enter the quantity: ";
        std::cin >> quantity;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid quantity. Try again.\n";
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        Order order (orderIdCounter++, price, quantity, Side::SELL);
        orderBook.addOrder(order, sellSide, buySide);
    });

    dispatcher.registerHandler(EventType::REMOVEORDER, removeHandler);

    dispatcher.registerHandler(EventType::ORDERHISTORY, [](const Event&) {
        std::cout << "Showing order history\n";
        std::cout << "Order History:\n";
        for (const auto& [id, order] : orderBook.orderHistory) {
            std::cout << "ID: " << order.getOrderId()
            << " | Price: " << order.getPrice()
            << " | Qty: " << order.getQuantity()
            << " | Side: " << (order.getSide() == Side::BUY ? "BUY" : "SELL") << '\n';
              }
  });

    dispatcher.registerHandler(EventType::DISPLAYORDERS, [](const Event&) {
        std::cout << "Displaying orders\n";
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
  });

    dispatcher.registerHandler(EventType::UNKNOWN, [](const Event&) {
    std::cout << "Unknown command. Please try again.\n";
});

    dispatcher.registerHandler(EventType::QUIT, [](const Event&) {
       std::cout << "Quitting\n";
    });

    std::string input;
    bool running = true;

    while (running) {
        std::cout << "Enter command (add bid, add ask, remove order, order history, display orders, quit): ";
        std::getline(std::cin, input);
        EventType type = parseInput(input);
        Event event(type);
        dispatcher.dispatch(event);
        if (type == EventType::QUIT) {
            running = false;
        }
    }

    return 0;
}