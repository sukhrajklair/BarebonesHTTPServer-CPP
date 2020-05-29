#include <string>
#include <istream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <vector>
#include <iterator>
#include "httpServer.h"

httpServer::~httpServer(){
    //wait for Messages handler thread to finish
    _messagesHandlerThread.join();
}
void httpServer::start(){
    //start message handler in a separate thread
    _messagesHandlerThread = std::thread(&httpServer::messagesHandler, this);

    //run tcp server
    Run();
}
// Handler for when a message is received from the client
std::string httpServer::RespondToMessage(std::string msg)
{
	// Parse out the client's request string e.g. GET /index.html HTTP/1.1
	std::istringstream iss(msg);
	std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

	// Some defaults for output to the client (404 file not found 'page')
	std::string content = "<h1>404 Not Found</h1>";
	std::string htmlFile = "/index.html";
	int errorCode = 404;

	// If the GET request is valid, try and get the name
	if (parsed.size() >= 3 && parsed[0] == "GET")
	{
		htmlFile = parsed[1];

		// If the file is just a slash, use index.html. This should really
		// be if it _ends_ in a slash. I'll leave that for you :)
		if (htmlFile == "/")
		{
			htmlFile = "/index.html";
		}
	}

	// Open the document in the local file system
    char filePath[100];
    realpath("./static",filePath);
	std::ifstream f(filePath + htmlFile);
	// Check if it opened and if it did, grab the entire contents
	if (f.good())
	{
		std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		content = str;
		errorCode = 200;
	}

	f.close();

	// Write the document back to the client
	std::ostringstream oss;
	oss << "HTTP/1.1 " << errorCode << " OK\r\n";
	oss << "Cache-Control: no-cache, private\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << content.size() << "\r\n";
	oss << "\r\n";
	oss << content;

	std::string output = oss.str();
	int size = output.size() + 1;
    return output;
}

void httpServer::messagesHandler(){
    while(true){
        //process next message from client if there is any(blocking)
        auto msg = nextMessage();
        //process the message according to HTTP protocol and form a response
        auto response = RespondToMessage(msg.second);
        //send response back the the client
        Send(msg.first, response);
    }
}