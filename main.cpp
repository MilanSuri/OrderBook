#include <iostream>
#include "orderbook.hpp"
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>

OrderBook orderBook;
OrderBookBuySide buySide;
OrderBookSellSide sellSide;
std::mutex orderBookMutex;

std::atomic<int> orderIdCounter(1);

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
    explicit Event(EventType t) : type(t) {}
};

class EventDispatcher {
    using Handler = std::function<void(const Event&)>;
    std::unordered_map<EventType, Handler> handlers;

public:
    void registerHandler(EventType type, Handler handler) {
        handlers[type] = handler;
    }

    void dispatch(const Event& event) {
        auto it = handlers.find(event.type);
        if (it != handlers.end()) {
            it->second(event);
        } else {
            std::cout << "No handler for this event \n";
        }
    }
};

std::string toLower(std::string s) {
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

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        stop_ = false;
        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        cv_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    try {
                        task();
                    } catch (const std::exception& e) {
                        std::cerr << "Task threw exception: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Task threw an unknown exception" << std::endl;
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;  // Set stop flag
        }
        cv_.notify_all();  // Notify all threads to wake up and stop

        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();  // Wait for all threads to finish
            }
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::move(task));  // Add task to queue
        }
        cv_.notify_one();  // Notify one thread to pick up the task
    }

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_;
};

int main() {
    ThreadPool threadPool(4);  // Create a pool with 4 threads
    EventDispatcher dispatcher;

    // Register event handlers
    dispatcher.registerHandler(EventType::ADDBID, [&threadPool](const Event&) {
        double price, quantity;
        std::cout << "Adding bid\n";
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
        threadPool.enqueue([price, quantity]() {
            std::lock_guard<std::mutex> lock(orderBookMutex);  // Protect shared resources
            Order order(orderIdCounter.fetch_add(1), price, quantity, Side::BUY);
            orderBook.addOrder(order, sellSide, buySide);  // Add the order to the order book
            std::cout << "Bid added: Price = " << price << ", Quantity = " << quantity << "\n";
        });
    });

    dispatcher.registerHandler(EventType::ADDASK, [&threadPool](const Event&) {
        double price, quantity;
        std::cout << "Adding ask\n";
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
        threadPool.enqueue([price, quantity]() {
            std::lock_guard<std::mutex> lock(orderBookMutex);
            Order order(orderIdCounter.fetch_add(1), price, quantity, Side::SELL);
            orderBook.addOrder(order, sellSide, buySide);
            std::cout << "Ask added: Price = " << price << ", Quantity = " << quantity << std::endl;
        });
    });

    dispatcher.registerHandler(EventType::REMOVEORDER, [&threadPool](const Event&) {
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

        threadPool.enqueue([orderId]() {
            std::lock_guard<std::mutex> lock(orderBookMutex);  // Protect shared resources
            bool removed = orderBook.removeOrderById(orderId, buySide, sellSide);
            if (removed) {
                std::cout << "Order " << orderId << " removed successfully.\n";
            } else {
                std::cout << "Failed to remove order " << orderId << ".\n";
            }
        });
    });

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
            std::cout << "Price: " << price << " | Total Qty: " << level.totalQuantity << '\n';
        }
        std::cout << "Active Sell Orders:\n";
        for (const auto& [price, level] : sellSide.asks) {
            std::cout << "Price: " << price << " | Total Qty: " << level.totalQuantity << '\n';
        }
    });

    // Main event loop
    while (true) {
        std::cout << "Enter command (add bid, add ask, remove order, order history, display orders, quit): ";
        std::string input;
        std::getline(std::cin, input);
        EventType eventType = parseInput(input);

        if (eventType == EventType::QUIT) {
            std::cout << "Exiting program...\n";
            break;
        }

        Event event(eventType);
        dispatcher.dispatch(event);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
