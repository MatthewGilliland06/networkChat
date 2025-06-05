#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

char *client_password = NULL;
int client_socket;

void waitForConnection(int port, bool passRequired, const std::string& password, const std::string& hostName, std::string& clientName)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (server_fd < 0) 
    {
        perror("socket");
        exit(1);
    }

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    listen(server_fd, 1);
    std::cout << "Waiting for a connection on port " << port << "..." << std::endl;

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0)
    {
        perror("accept");
        exit(1);
    }

    // Step 1: Receive the client's name
    char nameBuffer[256];
    ssize_t nameLen = recv(client_fd, nameBuffer, sizeof(nameBuffer) - 1, 0);
    if (nameLen <= 0)
    {
        std::cerr << "Failed to receive client name." << std::endl;
        close(client_fd);
        return;
    }
    nameBuffer[nameLen] = '\0';
    clientName = std::string(nameBuffer);

    // Step 2: Tell client if password is needed
    std::string passwordPrompt = passRequired ? "PASSWORD_REQUIRED" : "NO_PASSWORD";
    send(client_fd, passwordPrompt.c_str(), passwordPrompt.size(), 0);

    // Step 3: If needed, receive and verify password
    if (passRequired)
    {
        char pwBuffer[256];
        ssize_t pwLen = recv(client_fd, pwBuffer, sizeof(pwBuffer) - 1, 0);
        if (pwLen <= 0)
        {
            std::cerr << "Failed to receive password." << std::endl;
            close(client_fd);
            return;
        }

        pwBuffer[pwLen] = '\0';
        std::string receivedPass(pwBuffer);

        if (receivedPass != password)
        {
            std::string reject = "REJECT";
            send(client_fd, reject.c_str(), reject.size(), 0);
            std::cout << "Client provided wrong password. Connection closed." << std::endl;
            close(client_fd);
            return;
        }

        std::string accept = "ACCEPT";
        send(client_fd, accept.c_str(), accept.size(), 0);
    }

    // Step 4: Final connection established
    std::string intro = hostName + " is now connected with " + clientName + ".";
    send(client_fd, intro.c_str(), intro.size(), 0);

    client_socket = client_fd;
    close(server_fd);
    std::cout << "Client '" << clientName << "' connected successfully." << std::endl;
}

void sendLoop() {
    std::string message;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);

        if (message == "/exit") {
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
        std::cout << std::endl << "[Client]: " << buffer << std::endl << "> ";
        std::cout.flush();
    }
}


int main()
{
    std::string name = "";
    std::cout << "Enter the name you would like to appear as: ";
    std::cin >> name;
    
    bool passRequired = false;
    std::string password = "";
    std::cout << "Enter a password (Leave blank if you don't want one): ";
    std::cin >> password;

    if(password!="")
    {
        passRequired = true;
    }

    int port;
    std::cout << "Enter port to host on: ";
    std::cin >> port;
    std::cin.ignore();
    
    std::string clientName;

    waitForConnection(port, passRequired, password, name, clientName);

    std::thread recvThread(receiveLoop);

    std::thread sendThread(sendLoop);
    recvThread.join();
    sendThread.join();

    

    return 0;
}