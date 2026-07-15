#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

// --- Data Structures ---

struct Packet {
    char srcIP[16];
    char destIP[16];
    char payload[256];
    int ttl;
};

struct Route {
    std::string destinationSubnet;
    std::string nextHopIP;
    int port; // Simulating physical interface mapping
    int metric;
    bool isLinkActive;
};

// --- Core Simulator Engine ---

class VirtualRouter {
private:
    std::string routerIP;
    int listenPort;
    int sockfd;
    
    std::unordered_map<std::string, Route> routingTable;
    std::unordered_map<std::string, std::string> arpTable; // IP to simulated MAC/Port
    
    std::queue<Packet> packetQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondVar;
    bool running;

public:
    VirtualRouter(std::string ip, int port) : routerIP(ip), listenPort(port), running(true) {
        setupSocket();
    }

    ~VirtualRouter() {
        running = false;
        queueCondVar.notify_all();
        close(sockfd);
    }

    // 1. Socket Setup (Linux)
    void setupSocket() {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket for router " << routerIP << "\n";
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(listenPort);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Bind failed on port " << listenPort << "\n";
            exit(EXIT_FAILURE);
        }
        std::cout << "[INIT] Router " << routerIP << " listening on port " << listenPort << "\n";
    }

    // 2. Routing Table & ARP Management
    void addRoute(std::string dest, std::string nextHop, int outPort, int metric) {
        routingTable[dest] = {dest, nextHop, outPort, metric, true};
        std::cout << "[ROUTE ADDED] Dest: " << dest << " | Next Hop: " << nextHop << " | Port: " << outPort << "\n";
    }

    void updateARP(std::string ip, std::string mac) {
        arpTable[ip] = mac;
    }

    void simulateLinkFailure(std::string dest) {
        if (routingTable.find(dest) != routingTable.end()) {
            routingTable[dest].isLinkActive = false;
            std::cout << "[LINK DOWN] Route to " << dest << " is now inactive. Triggering convergence...\n";
        }
    }

    // 3. Packet Queuing and Receiving Thread
    void receiveLoop() {
        char buffer[sizeof(Packet)];
        struct sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);

        while (running) {
            int n = recvfrom(sockfd, buffer, sizeof(Packet), MSG_WAITALL, 
                             (struct sockaddr *)&clientAddr, &len);
            if (n > 0) {
                Packet* p = reinterpret_cast<Packet*>(buffer);
                std::lock_guard<std::mutex> lock(queueMutex);
                packetQueue.push(*p);
                queueCondVar.notify_one();
            }
        }
    }

    // 4. TCP/IP Forwarding and ARP Resolution Thread
    void processQueue() {
        while (running) {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondVar.wait(lock, [this] { return !packetQueue.empty() || !running; });
            
            if (!running) break;

            Packet p = packetQueue.front();
            packetQueue.pop();
            lock.unlock();

            std::string dest(p.destIP);
            
            // TTL Check
            if (--p.ttl <= 0) {
                std::cout << "[DROP] Packet TTL expired for dest: " << dest << "\n";
                continue;
            }

            // Is it for this router?
            if (dest == routerIP) {
                std::cout << "[DELIVERED] Router " << routerIP << " received payload: " << p.payload << "\n";
                continue;
            }

            // Forwarding Logic
            if (routingTable.find(dest) != routingTable.end() && routingTable[dest].isLinkActive) {
                Route route = routingTable[dest];
                
                // ARP Resolution (Mock)
                std::string nextHopMAC = (arpTable.find(route.nextHopIP) != arpTable.end()) ? 
                                          arpTable[route.nextHopIP] : "FF:FF:FF:FF:FF:FF (Broadcast)";
                
                std::cout << "[FORWARDING] " << p.srcIP << " -> " << p.destIP 
                          << " | Next Hop: " << route.nextHopIP << " (" << nextHopMAC << ")\n";

                // Transmit over socket to next hop
                struct sockaddr_in nextHopAddr;
                memset(&nextHopAddr, 0, sizeof(nextHopAddr));
                nextHopAddr.sin_family = AF_INET;
                nextHopAddr.sin_port = htons(route.port);
                nextHopAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost for simulation

                sendto(sockfd, reinterpret_cast<const char*>(&p), sizeof(Packet), 0, 
                       (const struct sockaddr *)&nextHopAddr, sizeof(nextHopAddr));
            } else {
                std::cout << "[DROP] No route to host or link failure: " << dest << "\n";
            }
        }
    }

    // Start threads
    void start() {
        std::thread rxThread(&VirtualRouter::receiveLoop, this);
        std::thread txThread(&VirtualRouter::processQueue, this);
        rxThread.detach();
        txThread.detach();
    }
};

// --- Test Implementation ---

int main() {
    // 1. Initialize Virtual Topology
    VirtualRouter routerA("192.168.1.1", 8001);
    VirtualRouter routerB("192.168.2.1", 8002);
    VirtualRouter routerC("192.168.3.1", 8003);

    // 2. Configure Routing Tables (A -> B -> C)
    routerA.addRoute("192.168.3.1", "192.168.2.1", 8002, 1); // A routes to C via B
    routerB.addRoute("192.168.3.1", "192.168.3.1", 8003, 0); // B routes directly to C

    // 3. Configure ARP Tables
    routerA.updateARP("192.168.2.1", "00:1A:2B:3C:4D:5E");
    routerB.updateARP("192.168.3.1", "00:1A:2B:3C:4D:5F");

    routerA.start();
    routerB.start();
    routerC.start();

    sleep(1); // Allow threads to initialize

    // 4. Inject a Packet into Router A destined for Router C
    std::cout << "\n--- Initiating Packet Transfer ---\n";
    int senderSock = socket(AF_INET, SOCK_DGRAM, 0);
    Packet testPacket = {"192.168.1.100", "192.168.3.1", "HELLO_FROM_CLIENT", 64};
    
    struct sockaddr_in rA_Addr;
    memset(&rA_Addr, 0, sizeof(rA_Addr));
    rA_Addr.sin_family = AF_INET;
    rA_Addr.sin_port = htons(8001);
    rA_Addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sendto(senderSock, reinterpret_cast<const char*>(&testPacket), sizeof(Packet), 0, 
           (const struct sockaddr *)&rA_Addr, sizeof(rA_Addr));

    sleep(2);

    // 5. Test Link Failure Recovery Logic
    std::cout << "\n--- Simulating Link Failure ---\n";
    routerA.simulateLinkFailure("192.168.3.1");
    sendto(senderSock, reinterpret_cast<const char*>(&testPacket), sizeof(Packet), 0, 
           (const struct sockaddr *)&rA_Addr, sizeof(rA_Addr));

    sleep(2);
    close(senderSock);
    return 0;
}