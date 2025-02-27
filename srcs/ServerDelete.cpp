#include "ServerDelete.hpp"

void ServerDelete::handleDeleteRequest(Server &server, int clientFd, HttpRequestParser &parser)
{
    std::string filePath = ResolvePaths::resolveFilePath(server, parser);
    std::cout << "DELETE request for file: " << filePath << std::endl;
    std::map<std::string, std::string> headers = parser.getHeaders();

    if (headers.find("Transfer-Encoding") != headers.end())
    {
        std::cerr << "❌ 400 Bad Request: DELETE request should not have Transfer-Encoding." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 400);
        return;
    }

    if (headers.find("Expect") != headers.end() && headers["Expect"] == "100-continue")
    {
        std::cerr << "❌ 417 Expectation Failed: DELETE request should not use Expect: 100-continue." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 417);
        return;
    }

    if (headers.find("Content-Length") != headers.end())
    {
        std::cerr << "❌ 400 Bad Request: DELETE request should not have a body." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 400);
        return;
    }
    if (ResolvePaths::findLocation(server, filePath) && !ResolvePaths::isMethodAllowed(server, filePath, "DELETE", parser.getPath()))
    {
        std::cerr << "❌ Error: DELETE not allowed for this location." << std::endl;
        ServerErrors::handleErrors(server, clientFd, 405);
        return;
    }
    if (access(filePath.c_str(), F_OK) != 0)
    {
        ServerErrors::handleErrors(server, clientFd, 404);
        return;
    }
    if (remove(filePath.c_str()) == 0)
    {
        std::string response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html\r\n"
                               "Content-Length: 26\r\n\r\n"
                               "File successfully deleted.";
        if (send(clientFd, response.c_str(), response.size(), 0) == -1)
            std::cerr << "Error sending DELETE response: " << strerror(errno) << std::endl;

    }
    else
        ServerErrors::handleErrors(server, clientFd, 500);
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
}