# Simple Order Book System in C++

A console-based order book simulator in C++ that models the core mechanics of buy and sell order matching in financial markets.

---

## Overview

This program simulates a simplified order book where users can place buy (bid) and sell (ask) orders. It automatically matches orders based on price and quantity, simulating trade executions as in real financial markets.

---

## Program Flow

1. **Start:**  
   The program greets the user and displays the main menu.

2. **User Input:**  
   The user selects one of the following options:

| Option | Action               | Description                                    |
|--------|----------------------|------------------------------------------------|
| 0      | Exit                 | Terminates the program                          |
| 1      | Add Buy Order (Bid)  | Enter price and quantity to place a buy order |
| 2      | Add Sell Order (Ask) | Enter price and quantity to place a sell order|
| 3      | Remove Buy Order     | Remove an existing buy order by its order ID  |
| 4      | Remove Sell Order    | Remove an existing sell order by its order ID |
| 5      | Show Order History   | Display all orders ever placed with details    |
| 6      | Show Active Orders   | Display current active buy and sell orders     |

3. **Order Matching:**  
   - When a buy or sell order is added, the program attempts to match it against existing orders on the opposite side, following price-time priority (FIFO per price level).  
   - Trades are executed automatically, updating order quantities or removing orders when fully filled.  
   - Each executed trade is displayed with executed quantity and price.

4. **Order Book Update:**  
   - Partially filled or unmatched orders are added to the active order book on their respective side.  
   - Users can remove active orders by specifying their order ID.

5. **Repeat:**  
   The menu is displayed again until the user chooses to exit.

---

## Features

- **FIFO Matching:** Orders at each price level are matched in the order they were received.  
- **Automatic Trade Execution:** Orders are matched and executed automatically, updating quantities accordingly.  
- **Order History:** Maintains a complete history of all orders placed, including executed trades.  
- **Order Management:** Supports adding new orders, removing existing orders by ID, and viewing both active orders and full order history.

---

## Usage

Run the program and follow the on-screen prompts to add or remove buy/sell orders, view order books, and track executed trades.

---

## Author

**Milan Suri**  
May 24th, 2025
