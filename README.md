# TCP/IP Routing Protocol Simulator 🌐

A lightweight, multi-threaded C++ network simulator that models core TCP/IP packet forwarding, ARP resolution, and dynamic routing across a virtual topology on Linux environments.

##  Overview

This project simulates a network of virtual routers communicating over a simulated link layer. Instead of physical hardware, it utilizes POSIX UDP sockets (`SOCK_DGRAM`) bound to local ports to represent network interfaces. It accurately models packet queuing, Time-To-Live (TTL) expiration, ARP table lookups, and link-failure recovery.

##  Features

* **Socket-Based Node Communication:** Uses Linux sockets to simulate link-layer connections and packet transmission between virtual routers.
* **Multi-Threaded Architecture:** Employs `std::thread`, `std::mutex`, and `std::condition_variable` to handle asynchronous packet receiving and processing queues concurrently.
* **Routing Table Management:** Configurable routing tables that dictate next-hop logic based on destination IP addresses.
* **ARP Resolution Simulation:** Translates next-hop IP addresses to simulated MAC addresses before forwarding.
* **Link-Failure Recovery:** Includes mechanisms to simulate link downtime and dynamically drop or reroute traffic to prevent routing black holes.

##  Tech Stack

* **Language:** C++ (C++11/14)
* **OS:** Linux / Unix (Requires POSIX sockets)
* **Libraries:** `<thread>`, `<mutex>`, `<sys/socket.h>`, `<arpa/inet.h>`

##  Getting Started

### Prerequisites
You need a Linux environment with a C++ compiler (`g++`) installed.

### Compilation
Clone the repository and compile the source code using the following command. The `-pthread` flag is required for the multi-threading components.

```bash
g++ -std=c++11 -pthread router_sim.cpp -o router_sim
