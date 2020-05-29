#include "tcpListener.h"

class httpServer : protected TcpListener
{
public:

	httpServer(std::string ipAddress, int port) :
		TcpListener(ipAddress, port) { }
    virtual ~httpServer();
    void start();

protected:

	// Handler for when a message is received from the client
	std::string RespondToMessage(std::string msg);

    void messagesHandler();

private:
    std::thread _messagesHandlerThread;
};