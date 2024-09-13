#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
// #include<winsock2.h>
#include <netinet/in.h>
// #include<ws2tcpip.h>
#include <stdbool.h>
#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <time.h>
#include <unordered_set>
#include <sys/time.h>
// #include <Kernel/sys/_types/_fd_zero.h>
using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_TIME 600000  // 10 minutes in milliseconds
#define MAX_REQ 10
#define CSV_FILE "../log.csv"
#define BAN_HANDSHAKE_SEC 0
#define BAN_HANDSHAKE_uSEC 500

class RecentCounter {
public:
    queue<pair<long long, string> > q;
    unordered_map<string, int> NODE_BROW_COUNT;
    int numReq;
    // string IP_ADD;
    RecentCounter(){
        numReq = 0;
    }
    int ping(long long t, string browser) {
        q.push(make_pair(t, browser));
        int count = 0;
        int size = q.size();
        NODE_BROW_COUNT[browser]++;
        while (q.front().first < t - MAX_TIME) {
            NODE_BROW_COUNT[q.front().second]--;
            q.pop();
            count++;
        }
        numReq = size - count;
        return size - count;
    }
};

// Function to get the current time in milliseconds
long long current_time_millis() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// Function to extract the browser name from the User-Agent string
void extract_browser_name(char *user_agent, char *browser) {
    if (strstr(user_agent, "Chrome") != NULL) {
        strcpy(browser, "Chrome");
    } else if (strstr(user_agent, "Firefox") != NULL) {
        strcpy(browser, "Firefox");
    } else if (strstr(user_agent, "Safari") != NULL) {
        strcpy(browser, "Safari");
    } else if (strstr(user_agent, "Edge") != NULL) {
        strcpy(browser, "Edge");
    } else {
        strcpy(browser, "Unknown");
    }
}

// Function to log the data to a CSV file
void log_request(char *ip_address, char *browser, long long time_millis) {
    FILE *file = fopen(CSV_FILE, "r+");
    if (!file) {
        file = fopen(CSV_FILE, "w");
        fprintf(file, "IP Address,Browser,Request Count,Time (ms)\n");
    } else {
        // Check if the entry exists
        char line[BUFFER_SIZE];
        char file_ip[BUFFER_SIZE];
        char file_browser[BUFFER_SIZE];
        int file_count;
        long long file_time;
        bool found = false;

        while (fgets(line, sizeof(line), file)) {
            sscanf(line, "%[^,],%[^,],%d,%lld", file_ip, file_browser, &file_count, &file_time);
            if (strcmp(file_ip, ip_address) == 0 && strcmp(file_browser, browser) == 0) {
                found = true;
                fseek(file, -strlen(line), SEEK_CUR);
                fprintf(file, "%s,%s,%d,%lld\n", file_ip, file_browser, file_count + 1, time_millis);
                break;
            }
        }

        if (!found) {
            fprintf(file, "%s,%s,%d,%lld\n", ip_address, browser, 1, time_millis);
        }
    }

    fclose(file);
}

bool user_agent(struct sockaddr_in *address, char *buffer, unordered_map<string, RecentCounter> &ipMap) {
    char *user_agent_start = strstr(buffer, "User-Agent:");

    printf("C analyzer: User-Agent %s", user_agent_start);
    if (user_agent_start != NULL) {
        user_agent_start += strlen("User-Agent:");  // Move the pointer past "User-Agent:"
        char *user_agent_end = strstr(user_agent_start, "\r\n");
        if (user_agent_end) {
            size_t length = user_agent_end - user_agent_start;
            char user_agent[length + 1];
            strncpy(user_agent, user_agent_start, length);
            user_agent[length] = '\0';  // Null-terminate the string

            char browser[BUFFER_SIZE];
            extract_browser_name(user_agent, browser);

            char ip_address[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(address->sin_addr), ip_address, INET_ADDRSTRLEN);

            long long current_time = current_time_millis();
            log_request(ip_address, browser, current_time);
            printf("C analyzer: IP %s, Browser %s logged at %lld ms.\n", ip_address, browser, current_time);
            string newStr(ip_address);
            string newBrow(browser);
            ipMap[newStr].ping(current_time, newBrow);
            return true;
        }
    }
    char ip_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address->sin_addr), ip_address, INET_ADDRSTRLEN);
    long long current_time = current_time_millis();
    char *valueStr = "NOT-FOUND";
    log_request(ip_address, valueStr, current_time);
    printf("\nC analyzer: User-Agent not found. Logged at %lld ms.\n", current_time);
    string newStr(ip_address);
    string newBrow("NOT_FOUND");
    ipMap[newStr].ping(current_time, newBrow);
    return false;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Attaching socket to the port 8081
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding the socket to the address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // RecentCounter counter;
    fd_set readfds;
    struct timeval timeout;
    unordered_map<string, RecentCounter> ipMap;
    unordered_set<string> blackList;
    // blackList.insert("127.0.0.1");
    while (1) {
        printf("C analyzer waiting for a connection...\n");
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        printf("C analyzer: Connection established.\n");

        char *client_ip = inet_ntoa(address.sin_addr);
        printf("Client IP: %s\n", client_ip);
        string client_ip_str(client_ip);

        // Debugging: Print the blacklist and the current IP being checked
        cout << "Current IP: " << client_ip_str << endl;
        cout << "Blacklist contents:" << endl;
        for (const auto &ip : blackList) {
            cout << ip << endl;
        }

        // Check if the client IP is in the blacklist
        if (blackList.find(client_ip_str) != blackList.end()) {
            const char *badresponse =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 6\r\n"
                "Connection: close\r\n"
                "\r\n"
                "unsafe";
            send(new_socket, badresponse, strlen(badresponse), 0);
            close(new_socket);  // Close the connection immediately
            continue;  // Skip the rest of the loop and wait for the next connection
        }

        FD_ZERO(&readfds);
        FD_SET(new_socket, &readfds);
        timeout.tv_sec = BAN_HANDSHAKE_SEC;  // 5-second timeout for read operations
        timeout.tv_usec = BAN_HANDSHAKE_uSEC;

        int activity = select(new_socket + 1, &readfds, NULL, NULL, &timeout);

        if (activity == 0) {
            // Timeout occurred, client didn't send any data
            printf("C analyzer: Client %s performed handshake but sent no data. Closing connection.\n", client_ip);
            close(new_socket);
            continue;
        } else if (activity < 0) {
            perror("select error");
            close(new_socket);
            continue;
        }

        int bytes_read = read(new_socket, buffer, BUFFER_SIZE);

        if (bytes_read <= 0) {
            // Client sent no data or an error occurred
            printf("C analyzer: Client %s sent no data after connection. Closing connection.\n", client_ip);
            close(new_socket);
            continue;
        }

        read(new_socket, buffer, BUFFER_SIZE);
        printf("C analyzer received: %s\n", buffer);

        // Initialize RecentCounter for new IPs
        if (ipMap.count(client_ip_str) == 0) {
            ipMap[client_ip_str] = RecentCounter();
        }

        // Check if the IP has exceeded the request limit
        if (ipMap[client_ip_str].numReq > MAX_REQ) {
            blackList.insert(client_ip_str);  // Add the IP to the blacklist
            // Send an unsafe response and close the connection
            const char *badresponse =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 6\r\n"
                "Connection: close\r\n"
                "\r\n"
                "unsafe";
            send(new_socket, badresponse, strlen(badresponse), 0);
            close(new_socket);
            continue;
        }

        // Process the User-Agent and log the request
        if (user_agent(&address, buffer, ipMap)) {
            const char *saferesponse =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 4\r\n"
                "Connection: close\r\n"
                "\r\n"
                "safe";
            send(new_socket, saferesponse, strlen(saferesponse), 0);
        } else {
            const char *badresponse =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 6\r\n"
                "Connection: close\r\n"
                "\r\n"
                "unsafe";
            send(new_socket, badresponse, strlen(badresponse), 0);
        }

        close(new_socket);
    }

    return 0;
}