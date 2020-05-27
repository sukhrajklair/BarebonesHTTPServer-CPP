#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

using namespace std;

int main(){
    /*create a socket*/
    //use ipv4, set type of socket to stream, set protocol to 0 so that it'll automatically detect the porotocol
    int listening = socket(AF_INET, SOCK_STREAM, 0); 

    //check if the socket is working
    if (listening == -1)
    {
        cerr << "can't create a socket!";
        return -1;
    }

    /*bind the socket to a IP/port*/
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000); //htons(), stands for host to network, convert the number to the proper "networkd byte order (Big-Endian) regardless of the hardware, 
    //inet_pton - convert IPv4 and IPv6 addresses from text to binary form
    //passing "0.0.0.0" generate any correct address
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);
    //bind address to the socket, the address needs to be cast from sockaddr_in to sockaddr
    if(bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1)
    {
        cerr << "Can't bind to IP/port";
        return -2;
    }

    //Mark the socket for listening in
    if(listen(listening, SOMAXCONN) == -1)
    {
        cerr << "Can't listen";
        return -3;
    }
    //accept a call
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    //buffers to store host name and service name
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];
    //await for connection, when connected put connecting peer's address and address length into client and clientSize respectively
    int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
    if (clientSocket == -1)
    {
        cerr << "Problem with Client connecting";
        return -4;
    }
    /*close the listening socket */
    close(listening);

    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);

    //translate socket address of client to a location and service name
    int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0);

    if(result)
    {
        cout << host << " connected on " << svc << endl;
    }
    else
    {   
        //if getnameinfo() wasn't successfull, then convert the network address manually to buffer using ntop
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        cout << host << " connected on " << ntohs(client.sin_port) << endl;
    }
    
    /*while receving - display message, echo message*/
    char buf[4096];
    while (true){
        //clear buffer
        memset(buf, 0, 4096);
        //wait for a message
        int bytesRecv = recv(clientSocket, buf, 4096, 0);
        if(bytesRecv == -1)
        {
            cerr << "There was a connection issue" << endl;
            break;
        }

        if (bytesRecv == 0)
        {
            cout << "The client disconnected" << endl;
            break;
        }

        //display message
        cout << "Received: " << string(buf, 0, bytesRecv) << endl;
        //resend message
        send(clientSocket, buf, bytesRecv + 1, 0);
        
        
    }
    /*close client socket */
    close(clientSocket);
    return 0;
}