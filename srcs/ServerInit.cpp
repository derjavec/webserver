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

std::string ServerInit::handleRedirections(Server &server, HttpRequestParser &parser)
{
    std::string externalRedirect;

    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it)
    {
        if (ResolvePaths::normalizePath(parser.getPath()) == ResolvePaths::normalizePath(it->getPath()) && !it->getRedirect().empty())
        {
            const std::string &redirectTarget = ResolvePaths::extractPathFromURL(it->getRedirect());
            for (std::vector<LocationConfig>::const_iterator locIt = server._locations.begin(); locIt != server._locations.end(); ++locIt)
            {
                if (ResolvePaths::normalizePath(redirectTarget) == ResolvePaths::normalizePath(locIt->getPath()))
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
