#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <queue>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

struct PrintJob {
    char clientName[51];     // Nome do cliente (50 chars + \0)
    char content[101];       // Conteúdo (100 chars + \0)
    char timestamp[20];      // Data/hora (formato: DD/MM/AAAA HH:MM:SS)
    
    PrintJob() {
        memset(clientName, 0, sizeof(clientName));
        memset(content, 0, sizeof(content));
        memset(timestamp, 0, sizeof(timestamp));
    }
};

class PrintServer {
private:
    SOCKET serverSocket;
    SOCKET clientSocket;
    std::queue<PrintJob> printQueue;
    int maxQueueSize;
    bool clientConnected;
    std::string outputFile;
    
    std::string getCurrentTimestamp() {
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);
        
        std::stringstream ss;
        ss << std::setfill('0') 
           << std::setw(2) << timeinfo->tm_mday << "/"
           << std::setw(2) << (timeinfo->tm_mon + 1) << "/"
           << (timeinfo->tm_year + 1900) << " "
           << std::setw(2) << timeinfo->tm_hour << ":"
           << std::setw(2) << timeinfo->tm_min << ":"
           << std::setw(2) << timeinfo->tm_sec;
        
        return ss.str();
    }
    
    void flushQueueToFile() {
        std::ofstream file(outputFile.c_str(), std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Erro: Não foi possível abrir o arquivo de impressão!" << std::endl;
            return;
        }
        
        std::cout << "Descarregando fila de impressão (" << printQueue.size() << " itens)..." << std::endl;
        
        while (!printQueue.empty()) {
            PrintJob job = printQueue.front();
            printQueue.pop();
            
            file << job.clientName << " | " << job.content << " | " << job.timestamp << std::endl;
        }
        
        file.close();
        std::cout << "Fila descarregada com sucesso!" << std::endl;
    }
    
    std::string processMessage(const std::string& message) {
        if (message.substr(0, 6) == "PRINT:") {
            // Formato: PRINT:NOME_CLIENTE:CONTEUDO
            size_t firstColon = message.find(':', 6);
            if (firstColon == std::string::npos) {
                return "ERRO: Formato inválido para impressão";
            }
            
            std::string clientName = message.substr(6, firstColon - 6);
            std::string content = message.substr(firstColon + 1);
            
            if (clientName.length() > 50) {
                return "ERRO: Nome do cliente muito longo (máximo 50 caracteres)";
            }
            
            if (content.length() > 100) {
                return "ERRO: Conteúdo muito longo (máximo 100 caracteres)";
            }
            
            PrintJob job;
            strncpy(job.clientName, clientName.c_str(), sizeof(job.clientName) - 1);
            job.clientName[sizeof(job.clientName) - 1] = '\0';
            strncpy(job.content, content.c_str(), sizeof(job.content) - 1);
            job.content[sizeof(job.content) - 1] = '\0';
            strncpy(job.timestamp, getCurrentTimestamp().c_str(), sizeof(job.timestamp) - 1);
            job.timestamp[sizeof(job.timestamp) - 1] = '\0';
            
            printQueue.push(job);
            
            std::cout << "Trabalho de impressão adicionado: " << clientName << " - " << content << std::endl;
            
            // Verifica se a fila atingiu o limite
            if (printQueue.size() >= maxQueueSize) {
                flushQueueToFile();
            }
            
            return "OK: Trabalho de impressão adicionado à fila";
            
        } else if (message == "LIST") {
            // Listar conteúdo da fila
            if (printQueue.empty()) {
                return "FILA_VAZIA: Não há trabalhos na fila de impressão";
            }
            
            std::stringstream response;
            response << "FILA:" << printQueue.size() << ":";
            
            std::queue<PrintJob> tempQueue = printQueue;
            int count = 1;
            while (!tempQueue.empty()) {
                PrintJob job = tempQueue.front();
                tempQueue.pop();
                
                response << count << "." << job.clientName << " - " 
                        << job.content << " (" << job.timestamp << ")";
                if (!tempQueue.empty()) response << "|";
                count++;
            }
            
            return response.str();
            
        } else {
            return "ERRO: Comando não reconhecido. Use PRINT:NOME:CONTEUDO ou LIST";
        }
    }
    
public:
    PrintServer(int port, int queueSize, const std::string& filename) 
        : maxQueueSize(queueSize), clientConnected(false), outputFile(filename) {
        
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
            throw std::runtime_error("Erro ao inicializar Winsock");
        }
        
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Erro ao criar socket");
        }
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Erro ao fazer bind do socket");
        }
        
        if (listen(serverSocket, 1) == SOCKET_ERROR) {
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Erro ao colocar socket em modo listen");
        }
        
        std::cout << "Servidor de Impressão iniciado na porta " << port << std::endl;
        std::cout << "Tamanho máximo da fila: " << maxQueueSize << std::endl;
        std::cout << "Arquivo de saída: " << outputFile << std::endl;
        std::cout << "Aguardando conexões..." << std::endl;
    }
    
    ~PrintServer() {
        if (clientConnected) {
            closesocket(clientSocket);
        }
        closesocket(serverSocket);
        WSACleanup();
        
        // Descarrega fila restante ao encerrar
        if (!printQueue.empty()) {
            std::cout << "Descarregando fila restante..." << std::endl;
            flushQueueToFile();
        }
    }
    
    void run() {
        while (true) {
            if (!clientConnected) {
                std::cout << "Aguardando nova conexão..." << std::endl;
                
                sockaddr_in clientAddr;
                int clientAddrSize = sizeof(clientAddr);
                clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
                
                if (clientSocket == INVALID_SOCKET) {
                    std::cerr << "Erro ao aceitar conexão" << std::endl;
                    continue;
                }
                
                char clientIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
                
                std::cout << "Cliente conectado: " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
                clientConnected = true;
                
                // Envia mensagem de boas-vindas
                std::string welcome = "CONECTADO: Bem-vindo ao Servidor de Impressão Remota";
                send(clientSocket, welcome.c_str(), welcome.length(), 0);
            }
            
            // Recebe mensagens do cliente
            char buffer[512];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived <= 0) {
                std::cout << "Cliente desconectado." << std::endl;
                closesocket(clientSocket);
                clientConnected = false;
                continue;
            }
            
            buffer[bytesReceived] = '\0';
            std::string message(buffer);
            
            std::cout << "Mensagem recebida: " << message << std::endl;
            
            // Processa a mensagem e envia resposta
            std::string response = processMessage(message);
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    }
};

int main() {
    try {
        int port, queueSize;
        std::string filename;
        
        std::cout << "=== SERVIDOR DE IMPRESSÃO REMOTA ===" << std::endl;
        std::cout << "Digite a porta do servidor: ";
        std::cin >> port;
        
        std::cout << "Digite o tamanho máximo da fila: ";
        std::cin >> queueSize;
        
        std::cout << "Digite o nome do arquivo de impressão: ";
        std::cin >> filename;
        
        PrintServer server(port, queueSize, filename);
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        system("pause");
        return 1;
    }
    
    return 0;
}
