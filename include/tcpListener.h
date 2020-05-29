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
#include <deque>

//following class tempelate is for a concurrent message que
//all of messages received from each client will be put in this que
//and processed one by one
template <class T>
class MessageQueue
{
public:
	T receive();
    void send( T && msg);
private:
	std::mutex _mutex;
    std::condition_variable _cond;
	std::deque<T> _messages;
};

class TcpListener{
    public:
        TcpListener(std::string ipAddress, int port);
        virtual ~TcpListener();

        void Send(int clientSocket, std::string msg);

        void Run();
    protected:
        std::pair<int, std::string> nextMessage();
    private:
        int CreateSocket();

        std::tuple<int, sockaddr_in, socklen_t>  WaitForConnection(int listening);

        void Communicate(std::tuple<int, sockaddr_in, socklen_t> && clientInfo);
        
        std::string ClientInfo(std::tuple<int, sockaddr_in, socklen_t> clientInfo);

        std::vector<std::future<void>> _clients;
        int _listening;
        std::string _ipAddress;
        int _port;
        //message que stores each message as pair of clientSocket number and the message
        MessageQueue<std::pair<int, std::string>> _messages;
};

#endif