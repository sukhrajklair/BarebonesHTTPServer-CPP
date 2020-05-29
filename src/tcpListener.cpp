#include "tcpListener.h"
#include <algorithm>
#include <sstream>

#define MAX_BUFFER_SIZE (49152)

/* Implementation of class "MessageQueue" */
template <typename T>
T MessageQueue<T>::receive()
{
    // The method receive uses std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
  	std::unique_lock<std::mutex> uLock(_mutex);
  	_cond.wait(uLock, [this]{return !(_messages.empty());});
  
  	T msg = std::move(_messages.back());
  	_messages.pop_back();
  
  	return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // The method send uses the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
  	std::lock_guard<std::mutex> lock(_mutex);
  	_messages.push_back(std::move(msg));
  	_cond.notify_one();
}


/*Implementation of TCP Listener*/
TcpListener::TcpListener(std::string ipAddress, int port)
    :_ipAddress(ipAddress), _port(port)
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
            //when a new connection is received, maintain it in a new thread
            _clients.emplace_back(std::async(&TcpListener::Communicate, this, std::move(clientInfo)));
        }
    
    }
}
//this method maintains the connection with a client
//it puses any messages received to the concurrent message que
void TcpListener::Communicate(std::tuple<int, sockaddr_in, socklen_t>&& clientInfo)
{   
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
            
            std::pair<int, std::string> message = std::make_pair( clientSocket, std::string(buf, 0, bytesReceived));
            _messages.send(std::move(message));
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

std::pair<int, std::string> TcpListener::nextMessage(){
    return _messages.receive();
}