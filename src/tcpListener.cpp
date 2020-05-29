#include "tcpListener.h"
#include <algorithm>
#include <sstream>

#define MAX_BUFFER_SIZE (49152)

TcpListener::TcpListener(std::string ipAddress, int port, MessageReceivedHandler handler)
    :_ipAddress(ipAddress), _port(port), _messageReceived(handler)
{

}
TcpListener::~TcpListener()
{
    //wait for all client connections to close
    std::for_each(_clients.begin(), _clients.end(), [](auto& futr) {
        futr.wait();
    });
    //close listening socket
    close(_listening);
}

void TcpListener::Send(int clientSocket, std::string msg)
{
    send(clientSocket, msg.c_str(), msg.size() + 1, 0);
}

void TcpListener::Run()
{
    std::cout << "thread: " << std::this_thread::get_id() << std::endl;
    //create a listening socket
    _listening = CreateSocket();
    if (_listening < 0)
    {
        return;
    }
    while (true)
    {
        
        //wait for connection
        auto clientInfo = WaitForConnection(_listening);
        int clientSocket = std::get<0>(clientInfo);
        if (clientSocket != 1)
        {
            _clients.emplace_back(std::async(&TcpListener::Communicate, this, std::move(clientInfo)));
        }
    
    }
}
void TcpListener::Communicate(std::tuple<int, sockaddr_in, socklen_t>&& clientInfo)
{   
    std::cout << "thread: " << std::this_thread::get_id() << std::endl;
    std::string clientInfoStr = ClientInfo(clientInfo);
    std::cout << "Connected to " <<  clientInfoStr << std::endl;
    int clientSocket = std::get<0>(clientInfo);
    char buf[MAX_BUFFER_SIZE];
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
    std::cout << clientInfoStr << " disconnected" << std::endl;
    close(clientSocket);
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

std::tuple<int, sockaddr_in, socklen_t> TcpListener::WaitForConnection(int listening)
{
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    //await for connection, when connected put connecting peer's address and address length into client and clientSize respectively
    int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
    return std::make_tuple(clientSocket, client, clientSize);
}

std::string TcpListener::ClientInfo(std::tuple<int, sockaddr_in, socklen_t> clientInfo)
{
    //buffers to store host name and service name
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];
    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);
    std::ostringstream oss;
    auto clientAddr = std::get<1>(clientInfo);
    auto clientSize = std::get<2>(clientInfo);
    //translate socket address of client to a location and service name
    int result = getnameinfo((sockaddr*)&clientAddr, clientSize, host, NI_MAXHOST, svc, NI_MAXSERV, 0);

    if(result)
    {
        oss << host << " port: " << svc;
    }
    else
    {   
        //if getnameinfo() wasn't successfull, then convert the network address manually to buffer using ntop
        inet_ntop(AF_INET, &clientAddr.sin_addr, host, NI_MAXHOST);
        oss << host << " port: " << ntohs(clientAddr.sin_port);
    }

    return oss.str();
}

