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
#include <sqlite3.h>
#include <curl/curl.h>
#include "json.hpp"  // from https://github.com/nlohmann/json
using json = nlohmann::json;

OrderBook orderBook;
OrderBookBuySide buySide;
OrderBookSellSide sellSide;
std::mutex orderBookMutex;

std::atomic<int> orderIdCounter(1);

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

bool validateTicker(const std::string& ticker, const std::string& apiKey) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to init curl\n";
        return false;
    }

    std::string readBuffer;
    // Build URL for exact ticker search with Polygon API
    std::string url = "https://api.polygon.io/v3/reference/tickers?ticker=" + ticker +
                      "&market=stocks&active=true&apiKey=" + apiKey;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Curl request failed: " << curl_easy_strerror(res) << "\n";
        return false;
    }

    try {
        auto responseJson = json::parse(readBuffer);

        // Polygon returns results in "results" array
        if (responseJson.contains("results") && responseJson["results"].is_array()) {
            for (const auto& item : responseJson["results"]) {
                if (item.contains("ticker") && item["ticker"].get<std::string>() == ticker) {
                    // Optional: check if "active" is true to be sure
                    if (item.contains("active") && item["active"].get<bool>() == true) {
                        return true;  // Valid active ticker found
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << "\n";
    }

    return false;
}

enum class EventType {
    ADDBID,
    ADDASK,
    REMOVEORDER,
    ORDERHISTORY,
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

double parseDoubleWithCommas(const std::string& input) {
    std::string cleaned;
    cleaned.reserve(input.size());

    // Keep digits, '.', and '-'; skip commas
    std::copy_if(input.begin(), input.end(), std::back_inserter(cleaned),
                 [](char c) { return (std::isdigit(c) || c == '.' || c == '-'); });

    try {
        return std::stod(cleaned);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing number '" << input << "': " << e.what() << std::endl;
        return std::numeric_limits<double>::quiet_NaN();  // signal invalid
    }
}



int main() {
    orderBook.initializeDB();
    orderBook.loadsOrdersFromDB(sellSide, buySide);

    ThreadPool threadPool(4);
    EventDispatcher dispatcher;

    // Register ADDBID handler
    dispatcher.registerHandler(EventType::ADDBID, [&threadPool](const Event&) {
    std::string ticker;

    std::cout << "Adding bid\nEnter ticker: ";
    std::getline(std::cin, ticker);

        std::string apiKey = "INPUT_YOUR_API_KEY";



    if (validateTicker(ticker, apiKey)) {
        std::cout << "Ticker '" << ticker << "' is valid and active.\n";
    } else {
    std::cout << "Ticker '" << ticker << "' is invalid or inactive.\n";
    return;
    }
    if (ticker.empty()) {
        std::cout << "Invalid ticker. Try again.\n";
        return;
    }

    std::string priceStr;
    std::cout << "Enter price: ";
    std::getline(std::cin, priceStr);
    double price = parseDoubleWithCommas(priceStr);
    if (std::isnan(price)) {
        std::cout << "Invalid price. Try again.\n";
        return;
    }

    std::string quantityStr;
    std::cout << "Enter quantity: ";
    std::getline(std::cin, quantityStr);
    double quantity = parseDoubleWithCommas(quantityStr);
    if (std::isnan(quantity)) {
        std::cout << "Invalid quantity. Try again.\n";
        return;
    }

    threadPool.enqueue([price, quantity, ticker]() {
        std::lock_guard<std::mutex> lock(orderBookMutex);
        Order order(orderIdCounter.fetch_add(1), price, quantity, Side::BUY, ticker);
        orderBook.addOrder(order);
        std::cout << "Bid added: Ticker = "<< ticker <<", Price = " << price << ", Quantity = " << quantity << "\n";
    });
});


    // Register ADDASK handler
    dispatcher.registerHandler(EventType::ADDASK, [&threadPool](const Event&) {
        std::string ticker;
        std::string apiKey = "INPUT_YOUR_APIKEY";

        std::cout << "Adding ask\nEnter ticker: ";
        std::getline(std::cin, ticker);

        if (validateTicker(ticker, apiKey)) {
        std::cout << "Ticker '" << ticker << "' is valid and active.\n";
    } else {
        std::cout << "Ticker '" << ticker << "' is invalid or inactive.\n";
        return;
    }

        if (ticker.empty()) {
            std::cout << "Invalid ticker. Try again.\n";
            return;
        }

        std::string priceStr;
        std::cout << "Enter price: ";
        std::getline(std::cin, priceStr);
        double price = parseDoubleWithCommas(priceStr);
        if (std::isnan(price)) {
            std::cout << "Invalid price. Try again.\n";
            return;
        }

    std::string quantityStr;
    std::cout << "Enter quantity: ";
    std::getline(std::cin, quantityStr);
    double quantity = parseDoubleWithCommas(quantityStr);
    if (std::isnan(quantity)) {
        std::cout << "Invalid quantity. Try again.\n";
        return;
    }

    threadPool.enqueue([price, quantity, ticker]() {
        std::lock_guard<std::mutex> lock(orderBookMutex);
        Order order(orderIdCounter.fetch_add(1), price, quantity, Side::SELL, ticker);
        orderBook.addOrder(order);
        std::cout << "Ask added: Ticker = "<< ticker <<", Price = " << price << ", Quantity = " << quantity << "\n";
    });
});


    // Register REMOVEORDER handler
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
            std::lock_guard<std::mutex> lock(orderBookMutex);
            bool removed = orderBook.removeOrderById(orderId, buySide, sellSide);
            if (removed) {
                std::cout << "Order " << orderId << " removed successfully.\n";
            } else {
                std::cout << "Failed to remove order " << orderId << ".\n";
            }
        });
    });

    // Register ORDERHISTORY handler
    dispatcher.registerHandler(EventType::ORDERHISTORY, [](const Event&) {
        std::lock_guard<std::mutex> lock(orderBookMutex);
        orderBook.displayOrders();  // Note: DB must be accessible here
    });



    // Main event loop
    while (true) {
        std::cout << "Enter command (add bid, add ask, remove order, order history, quit): ";
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