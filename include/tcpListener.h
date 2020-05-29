#ifndef TCPLISTENER_H
#define TCPLISTENER_H

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <thread>
#include <vector>
#include <future>
#include <tuple>

//Forward declaration
class TcpListener;

typedef void(*MessageReceivedHandler)(TcpListener* listener, int socketId, std::string msg);

class TcpListener{
    public:
        TcpListener(std::string ipAddress, int port, MessageReceivedHandler handler);
        ~TcpListener();

        void Send(int clientSocket, std::string msg);

        void Run();

    private:
        int CreateSocket();

        std::tuple<int, sockaddr_in, socklen_t>  WaitForConnection(int listening);

        void Communicate(std::tuple<int, sockaddr_in, socklen_t> && clientInfo);
        
        std::string ClientInfo(std::tuple<int, sockaddr_in, socklen_t> clientInfo);

        std::vector<std::future<void>> _clients;
        int _listening;
        std::string _ipAddress;
        int _port;
        MessageReceivedHandler _messageReceived;

};

#endif