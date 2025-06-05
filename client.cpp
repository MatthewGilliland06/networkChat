#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int client_socket;

void sendLoop() {
    std::string message;
    while (true)
    {
        std::cout << "> ";
        std::string message;
        std::getline(std::cin, message);

        if (message == "/exit")
        {
            std::cout << "Exiting..." << std::endl;
            close(client_socket);
            exit(0);
        }

        send(client_socket, message.c_str(), message.size(), 0);
    }
}


void receiveLoop() {
    char buffer[1024];

    while (true) {
        ssize_t bytesReceived = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cout << std::endl << "Connection closed by peer." << std::endl;
            close(client_socket);
            exit(0);
        }

        buffer[bytesReceived] = '\0'; // Null-terminate the buffer
        std::cout << std::endl << "[Client]: " << buffer << std::endl;
        std::cout.flush();
    }
}

int main()
{
    std::string name;
    std::string ip;
    int port;

    std::cout << "Enter your name: ";
    std::getline(std::cin, name);

    std::cout << "Enter host IP address: ";
    std::getline(std::cin, ip);

    std::cout << "Enter port number: ";
    std::cin >> port;
    std::cin.ignore();

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        return 1;
    }

    // Send name to host first
    send(client_socket, name.c_str(), name.size(), 0);

    // Wait for password request or not
    char response[32];
    ssize_t len = recv(client_socket, response, sizeof(response) - 1, 0);
    if (len <= 0)
    {
        std::cerr << "Failed to receive response from host." << std::endl;
        close(client_socket);
        return 1;
    }

    response[len] = '\0';
    std::string serverResponse(response);

    if (serverResponse == "PASSWORD_REQUIRED")
    {
        std::string password;
        std::cout << "Enter password: ";
        std::getline(std::cin, password);
        send(client_socket, password.c_str(), password.size(), 0);

        char pwCheck[16];
        len = recv(client_socket, pwCheck, sizeof(pwCheck) - 1, 0);
        if (len <= 0)
        {
            std::cerr << "No response after sending password." << std::endl;
            close(client_socket);
            return 1;
        }

        pwCheck[len] = '\0';
        if (std::string(pwCheck) != "ACCEPT")
        {
            std::cout << "Incorrect password. Connection rejected." << std::endl;
            close(client_socket);
            return 1;
        }
    }

    std::thread recvThread(receiveLoop);
    std::thread sendThread(sendLoop);

    recvThread.join();
    sendThread.join();

    return 0;
}
