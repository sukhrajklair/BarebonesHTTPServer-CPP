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

//Forward declaration
class TcpListener;

typedef void(*MessageReceivedHandler)(TcpListener* listener, int socketId, std::string msg);

class TcpListener{
    public:
        TcpListener(std::string ipAddress, int port, MessageReceivedHandler handler);
        ~TcpListener();

        void Send(int clientSocket, std::string msg);

        void Run();

        void Cleanup();
    private:
        int CreateSocket();

        int WaitForConnection(int listening);

        std::string _ipAddress;
        int _port;
        MessageReceivedHandler _messageReceived;

};

#endif