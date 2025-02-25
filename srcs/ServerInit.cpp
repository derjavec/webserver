#include "ServerInit.hpp"


std::string ServerInit::handleCookies(Server &server, HttpRequestParser &parser, int clientFd)
{
    std::map<std::string, std::string> cookies = parser.getCookies();
    std::string sessionId;
    if (cookies.find("session_id") != cookies.end())
        sessionId = cookies["session_id"];
    else
        sessionId = ServerUtils::generateSessionId();
    if (server._clientSessions.find(clientFd) == server._clientSessions.end())
        server._clientSessions[clientFd].isLoggedIn = false;
    server._clientSessions[clientFd].sessionId = sessionId;        
    std::string setCookieHeader = "Set-Cookie: session_id=" + server._clientSessions[clientFd].sessionId + "; Path=/; HttpOnly\r\n"; 
    return (setCookieHeader);
}

void ServerInit::handleMultiPorts(Server&server, const std::vector<uint16_t>& ports)
{
    for (size_t i = 0; i < ports.size(); ++i)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
            throw ServerException("Failed to create socket: " + std::string(strerror(errno)));
        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags == -1 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            close(sockfd);
            throw ServerException("Failed to set socket non-blocking: " + std::string(strerror(errno)));
        }
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ports[i]);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            close(sockfd);
        	std::stringstream ss;
            ss << ports[i];
            throw ServerException("Failed to bind socket on port " + ss.str() + ": " + std::string(strerror(errno)));
        }
        if (listen(sockfd, SOMAXCONN) < 0)
        {
            close(sockfd);
        	std::stringstream ss;
            ss << ports[i];
            throw ServerException("Failed to listen on port " + ss.str() + ": " + std::string(strerror(errno)));
        }
        server._listeningSockets.push_back(sockfd);
        server._listeningAddresses.push_back(addr);
    }
}

// bool ServerInit::handleImplicitRedirections(Server &server, int clientFd, const std::string &filePath)
// {
//     std::string cleanPath = filePath;
//     while (cleanPath.size() > 1 && cleanPath[cleanPath.size()-1] == '/' && cleanPath[cleanPath.size()-2] == '/')
//         cleanPath.erase(cleanPath.size()-1);
//     if (cleanPath != filePath)
//     {
//         std::cout << "Redirecting to: " << cleanPath << std::endl;
//         std::string response = "HTTP/1.1 301 Moved Permanently\r\n";
//         response += "Location: " + cleanPath + "\r\n";
//         response += "Content-Length: 0\r\n\r\n";
//         send(clientFd, response.c_str(), response.size(), 0);
//         shutdown(clientFd, SHUT_WR);
//         close(clientFd);
//         return true;
//     }
    
    // if (filePath[filePath.size() - 1] != '/')
    // {
    //     std::string locIndex;
    //     bool autoindex = false;
    //     std::string dummy;
    //     if (ServerUtils::findLocationConfig(server, filePath, locIndex, autoindex, dummy))
    //     {
    //         if (!locIndex.empty())
    //             return false;
    //         std::string redirectPath = filePath + "/";
    //         std::cout << "Redirecting to: " << redirectPath << std::endl;
    //         std::string response = "HTTP/1.1 301 Moved Permanently\r\n";
    //         response += "Location: " + redirectPath + "\r\n";
    //         response += "Content-Length: 0\r\n\r\n";
    //         send(clientFd, response.c_str(), response.size(), 0);
    //         shutdown(clientFd, SHUT_WR);
    //         close(clientFd);
    //         return true;
    //     }
    // }
    
    // if (filePath.find("welcome.html") != std::string::npos)
    // {
    //     std::string redirectPath = "login.html";
    //     std::cout << "Redirecting to: " << redirectPath << std::endl;
    //     std::string response = "HTTP/1.1 301 Moved Permanently\r\n";
    //     response += "Location: " + redirectPath + "\r\n";
    //     response += "Content-Length: 0\r\n\r\n";
    //     send(clientFd, response.c_str(), response.size(), 0);
    //     shutdown(clientFd, SHUT_WR);
    //     close(clientFd);
    //     return true;
    // }
    
//     return false;
// }

std::string ServerInit::handleRedirections(Server &server, HttpRequestParser &parser)
{
    std::string externalRedirect;

    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it)
    {
        if (ServerUtils::normalizePath(parser.getPath()) == ServerUtils::normalizePath(it->getPath()) && !it->getRedirect().empty())
        {
            const std::string &redirectTarget = ServerUtils::extractPathFromURL(it->getRedirect());
            for (std::vector<LocationConfig>::const_iterator locIt = server._locations.begin(); locIt != server._locations.end(); ++locIt)
            {
                if (ServerUtils::normalizePath(redirectTarget) == ServerUtils::normalizePath(locIt->getPath()))
                {
                    parser.setPath(redirectTarget);
                    parser.setType(redirectTarget);
                    return (externalRedirect);
                }
            }
            externalRedirect = redirectTarget;
            return (externalRedirect);
        }
    }
    return (externalRedirect);
}
