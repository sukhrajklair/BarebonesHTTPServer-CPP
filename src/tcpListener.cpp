#include "tcpListener.h"

#define MAX_BUFFER_SIZE (49152)

TcpListener::TcpListener(std::string ipAddress, int port, MessageReceivedHandler handler)
    :_ipAddress(ipAddress), _port(port), _messageReceived(handler)
{

}
TcpListener::~TcpListener()
{
    Cleanup();
}

void TcpListener::Send(int clientSocket, std::string msg)
{
    send(clientSocket, msg.c_str(), msg.size() + 1, 0);
}

void TcpListener::Run()
{
    char buf[MAX_BUFFER_SIZE];

    while (true)
    {
        //create a listening socket
        int listening = CreateSocket();
        if (listening < 0)
        {
            break;
        }
        //wait for connection
        int clientSocket = WaitForConnection(listening);
        if (clientSocket != 1)
        {
            //close listening socket to prevent another connection
            close(listening);
            int bytesReceived = 0;
            do
            {
                memset(buf, 0 , MAX_BUFFER_SIZE);
                bytesReceived = recv(clientSocket, buf, MAX_BUFFER_SIZE, 0);
                if (bytesReceived > 0)
                {
                    if (_messageReceived != NULL)
                    {
                        _messageReceived(this, clientSocket, std::string(buf, 0, bytesReceived));
                    }
                }
            } while (bytesReceived > 0);
            std::cout << "Client Disconnected" << std::endl;
            close(clientSocket);
        }
    
    }
}

int TcpListener::CreateSocket()
{
    /*create a socket*/
    //use ipv4, set type of socket to stream, set protocol to 0 so that it'll automatically detect the porotocol
    int listening = socket(AF_INET, SOCK_STREAM, 0); 

    //check if the socket is working
    if (listening == -1)
    {
        std::cerr << "can't create a socket!";
        return -1;
    }

    /*bind the socket to a IP/port*/
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(_port); //htons(), stands for host to network, convert the number to the proper "networkd byte order (Big-Endian) regardless of the hardware, 
    //inet_pton - convert IPv4 and IPv6 addresses from text to binary form

    inet_pton(AF_INET, _ipAddress.c_str(), &hint.sin_addr);
    //bind address to the socket, the address needs to be cast from sockaddr_in to sockaddr
    if(bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1)
    {
        std::cerr << "Can't bind to IP/port";
        return -2;
    }

    //Mark the socket for listening in
    if(listen(listening, SOMAXCONN) == -1)
    {
        std::cerr << "Can't listen";
        return -3;
    }
    std::cout<< "listening on ip:" << _ipAddress << " and port:" << _port << std::endl;
    return listening;
}

int TcpListener::WaitForConnection(int listening)
{
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    //await for connection, when connected put connecting peer's address and address length into client and clientSize respectively
    int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

    //if client is connected, print their information
    if (clientSocket != -1)
    {
        //buffers to store host name and service name
        char host[NI_MAXHOST];
        char svc[NI_MAXSERV];
        memset(host, 0, NI_MAXHOST);
        memset(svc, 0, NI_MAXSERV);

        //translate socket address of client to a location and service name
        int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0);

        if(result)
        {
            std::cout << host << " connected on " << svc << std::endl;
        }
        else
        {   
            //if getnameinfo() wasn't successfull, then convert the network address manually to buffer using ntop
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            std::cout << host << " connected on " << ntohs(client.sin_port) << std::endl;
        }
    }
    return clientSocket;
}

