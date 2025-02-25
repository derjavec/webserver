#include "ServerErrors.hpp"

void ServerErrors::sendGenericErrorResponse(int clientFd, int code)
{
    std::string response = "HTTP/1.1 " + ServerUtils::numberToString(code) + " " + getStatusMessage(code) + "\r\n";
    std::string genericContent = "<html><body><h1>" + ServerUtils::numberToString(code) + " " + getStatusMessage(code) + "</h1></body></html>";
    
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + ServerUtils::numberToString(genericContent.size()) + "\r\n\r\n";
    response += genericContent;
    if (send(clientFd, response.c_str(), response.size(), 0) == -1)
        std::cerr << "Error sending generic " << code << " response to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
}

void ServerErrors::handleErrors(Server &server, int clientFd, int code)
{
    std::map<int, std::string>::const_iterator it = server._errorPages.find(code);
    if (it == server._errorPages.end())
    {
        std::cerr << "⚠️ Error code not found in configuration: " << code << std::endl;
        sendGenericErrorResponse(clientFd, code);
        return;
    }
    std::string errorFilePath = server._root + it->second;
    std::ifstream errorFile(errorFilePath.c_str());
    if (errorFile.is_open())
    {
        std::stringstream errorBuffer;
        errorBuffer << errorFile.rdbuf();
        std::string errorContent = errorBuffer.str();
        errorFile.close();

        std::string response = "HTTP/1.1 " + ServerUtils::numberToString(code) + " " + getStatusMessage(code) + "\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + ServerUtils::numberToString(errorContent.size()) + "\r\n\r\n";
        response += errorContent;

        if (send(clientFd, response.c_str(), response.size(), 0) == -1)
            std::cerr << " Error sending " << code << " error page to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
    }
    else
    {
        std::cerr << "⚠️ Error page not found: " << errorFilePath << std::endl;
        sendGenericErrorResponse(clientFd, code);
    }
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}

std::string ServerErrors::getStatusMessage(int code)
{
    switch (code)
    {
	    case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
	    case 405: return "Method Not Allowed";
	    case 413: return "Request Entity Too Large";
        case 500: return "Internal Server Error";
	    case 501: return "Not Implemented";
	    case 505: return "HTTP Version Not Supported";
        default: return "Error";
    }
}