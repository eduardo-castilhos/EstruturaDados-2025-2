#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class PrintClient {
private:
    SOCKET clientSocket;
    bool connected;
    
public:
    PrintClient() : connected(false) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
            throw std::runtime_error("Erro ao inicializar Winsock");
        }
        
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Erro ao criar socket");
        }
    }
    
    ~PrintClient() {
        if (connected) {
            closesocket(clientSocket);
        }
        WSACleanup();
    }
    
    bool connectToServer(const std::string& serverIP, int port) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "Erro: IP inválido" << std::endl;
            return false;
        }
        
        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Erro: Não foi possível conectar ao servidor" << std::endl;
            return false;
        }
        
        connected = true;
        std::cout << "Conectado ao servidor " << serverIP << ":" << port << std::endl;
        
        // Recebe mensagem de boas-vindas
        char buffer[512];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Servidor: " << buffer << std::endl;
        }
        
        return true;
    }
    
    void sendPrintJob(const std::string& clientName, const std::string& content) {
        if (!connected) {
            std::cout << "Erro: Não conectado ao servidor" << std::endl;
            return;
        }
        
        std::string message = "PRINT:" + clientName + ":" + content;
        send(clientSocket, message.c_str(), message.length(), 0);
        
        // Recebe resposta
        char buffer[512];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Servidor: " << buffer << std::endl;
        }
    }
    
    void requestQueueList() {
        if (!connected) {
            std::cout << "Erro: Não conectado ao servidor" << std::endl;
            return;
        }
        
        std::string message = "LIST";
        send(clientSocket, message.c_str(), message.length(), 0);
        
        // Recebe resposta
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string response(buffer);
            
            if (response.substr(0, 5) == "FILA:") {
                // Parse da lista da fila
                size_t firstColon = response.find(':', 5);
                if (firstColon != std::string::npos) {
                    int queueSize = std::stoi(response.substr(5, firstColon - 5));
                    std::string queueContent = response.substr(firstColon + 1);
                    
                    std::cout << "=== FILA DE IMPRESSÃO (" << queueSize << " itens) ===" << std::endl;
                    
                    // Substitui separadores por quebras de linha
                    size_t pos = 0;
                    while ((pos = queueContent.find('|', pos)) != std::string::npos) {
                        queueContent.replace(pos, 1, "\n");
                        pos += 1;
                    }
                    
                    std::cout << queueContent << std::endl;
                    std::cout << "=============================================" << std::endl;
                }
            } else if (response.substr(0, 10) == "FILA_VAZIA") {
                std::cout << "A fila de impressão está vazia." << std::endl;
            } else {
                std::cout << "Servidor: " << response << std::endl;
            }
        }
    }
    
    void run() {
        if (!connected) {
            std::cout << "Erro: Não conectado ao servidor" << std::endl;
            return;
        }
        
        std::string clientName;
        std::cout << "Digite seu nome de cliente: ";
        std::getline(std::cin, clientName);
        
        int option;
        do {
            std::cout << "\n=== MENU CLIENTE ===" << std::endl;
            std::cout << "1. Enviar trabalho de impressão" << std::endl;
            std::cout << "2. Listar fila de impressão" << std::endl;
            std::cout << "3. Sair" << std::endl;
            std::cout << "Escolha uma opção: ";
            std::cin >> option;
            std::cin.ignore(); // Limpa o buffer
            
            switch (option) {
                case 1: {
                    std::string content;
                    std::cout << "Digite o conteúdo para impressão: ";
                    std::getline(std::cin, content);
                    sendPrintJob(clientName, content);
                    break;
                }
                case 2:
                    requestQueueList();
                    break;
                case 3:
                    std::cout << "Desconectando..." << std::endl;
                    break;
                default:
                    std::cout << "Opção inválida!" << std::endl;
            }
            
        } while (option != 3);
    }
};

int main() {
    try {
        std::string serverIP;
        int port;
        
        std::cout << "=== CLIENTE DE IMPRESSÃO REMOTA ===" << std::endl;
        std::cout << "Digite o IP do servidor: ";
        std::cin >> serverIP;
        
        std::cout << "Digite a porta do servidor: ";
        std::cin >> port;
        std::cin.ignore(); // Limpa o buffer
        
        PrintClient client;
        if (client.connectToServer(serverIP, port)) {
            client.run();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        system("pause");
        return 1;
    }
    
    return 0;
}
