#include <iostream>
#include <cstring>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <algorithm>
#include <mutex>
#include <atomic>

const int PORT = 8080;

int server_fd = 0;
bool running = true;
std::vector<int> client_fds;
std::mutex vec_mutex;
std::vector<std::thread> client_threads;

void signalHandler(int signum)
{
    std::cout << "\nCaught signal " << signum << ". Shutting down server..." << std::endl;
    running = false;
}

void handleClient(int client_fd)
{
    // Create a buffer
    char buffer[1024] = {0};
    while (running)
    {
        std::string spam = std::to_string(running);
        int valsent = send(client_fd, spam.c_str(), spam.size(), 0);

        int valread = read(client_fd, buffer, sizeof(buffer) - 1);
        if (valread <= 0)
        {
            std::cerr << "Read from client failed or connection closed" << std::endl;
            break;
        }

        // Null-terminate the buffer based on bytes read
        buffer[valread] = '\0';

        // Print out receival in server log
        std::cout << "Received: " << buffer << std::endl;

        // Respond to client
        std::string response = "We received " + std::string(buffer);
        int valsent = send(client_fd, response.c_str(), response.size(), 0);
        if (valsent <= 0)
        {
            std::cerr << "Failed to send feedback to client" << std::endl;
        }
    }

    std::cout << "Served closed, closing client..." << std::endl;
    close(client_fd);
}

int main()
{
    signal(SIGINT, signalHandler);

    // Create a main socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // Instantialize the server address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Binding failed" << std::endl;
        close(server_fd);
        return -1;
    }

    // Listen for any connections
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Listen failed" << std::endl;
        close(server_fd);
        return -1;
    }

    while (running)
    {
        // Accept incoming connections
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0)
        {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        client_fds.push_back(client_fd);

        // Create a thread and store it in the vector
        client_threads.emplace_back(std::thread(handleClient, client_fd));
    }
    std::cout << "Running is false, cleaning up now." << std::endl;

    // Wait for all threads to finish
    for (auto &thread : client_threads)
    {
        if (thread.joinable())
        {
            thread.join(); // Wait for each thread to finish
        }
    }

    close(server_fd);
    std::cout << "Server shut down gracefully." << std::endl;
    return 0;
}