# Simple Order Book System in C++

A console-based order book simulator in C++ that models the core mechanics of buy and sell order matching in financial markets.

---

## Program Flow

1. **Start:** The program greets the user and displays the main menu.
2. **User Input:** The user selects one of the following options:

| Option | Action                             | Description                                   |
|--------|----------------------------------|-----------------------------------------------|
| 0      | Exit                             | Terminates the program                        |
| 1      | Add Buy Order (Bid)               | Enter price and quantity to place a buy order |
| 2      | Add Sell Order (Ask)              | Enter price and quantity to place a sell order |
| 3      | Remove Buy Order                  | Remove an existing buy order by its order ID  |
| 4      | Remove Sell Order                 | Remove an existing sell order by its order ID |
| 5      | Show Order History                | Display all orders ever placed with details    |
| 6      | Show Active Orders                | Display current active buy and sell orders     |

3. **Order Matching:**  
   - When a buy or sell order is added, the program attempts to match it against existing opposite-side orders based on price and quantity.
   - Trades are executed automatically, reducing or removing matched orders.
   - Executed trades are displayed with quantity and price details.

4. **Order Book Update:**  
   - Partially filled or unmatched orders are added to the respective buy or sell order book.
   - Orders can be removed manually by their order ID.

5. **Repeat:** The menu is shown again until the user chooses to exit.

---

## Features

- FIFO matching at each price level.
- Automatic trade execution and quantity updates.
- Maintains a history of all orders.
- Supports adding, removing, and viewing orders.
--
Built by Milan Suri - May 24th, 2025
