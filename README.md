# Order Book System (C++)

This is a basic implementation of an order book system using C++. It supports the management of buy and sell orders, including the matching of orders when prices meet. The system utilizes multi-threading, event-driven architecture, and thread pooling for concurrent order processing.

## Features

- **Order Book Management:**
  - Supports both buy and sell orders.
  - Orders are matched based on price and side (buy/sell).
  - FIFO (First-In-First-Out) matching for orders at the same price level.
  
- **Multi-Threading:**
  - A custom thread pool is used to handle concurrent tasks, ensuring non-blocking execution for order additions and removals.

- **Event-Driven Architecture:**
  - Commands are processed based on events, providing flexibility to extend the system with new events in the future.
 
- **API-Verification**
  - Verifies stock tickers with an API to ensure they are real tickers.
  
- **Order History Storage**
  - Adds new orders to an sqlite database.
