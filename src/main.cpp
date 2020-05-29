#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <sstream>

#include "tcpListener.h"

using namespace std;

void Listener_MessageReceived(TcpListener* listener, int client, std::string msg);

int main(int argc, char* argv[]){

    std::string ipAddress = argv[1];
    std::istringstream ss(argv[2]);
    int port;
    ss >> port;
    TcpListener server(ipAddress, port, Listener_MessageReceived);
    
    server.Run();

    return 0;
}
void Listener_MessageReceived(TcpListener* listener, int client, std::string msg)
{
    std::cout << "thread: " << std::this_thread::get_id() << std::endl;
    listener->Send(client, msg);
}
