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

void ServerInit::handleMultiPorts(Server&server)
{
    int opt_val = 1;
    for (size_t i = 0; i < server._ports.size(); ++i)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
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
        addr.sin_port = htons(server._ports[i]);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            close(sockfd);
        	std::stringstream ss;
            ss << server._ports[i];
            throw ServerException("Failed to bind socket on port " + ss.str() + ": " + std::string(strerror(errno)));
        }
        if (listen(sockfd, SOMAXCONN) < 0)
        {
            close(sockfd);
        	std::stringstream ss;
            ss << server._ports[i];
            throw ServerException("Failed to listen on port " + ss.str() + ": " + std::string(strerror(errno)));
        }
        server._listeningSockets.push_back(sockfd);
        server._listeningAddresses.push_back(addr);
    }
}

std::string extractPathFromURL(const std::string &url)
{
    size_t pos = url.find("://");
    if (pos != std::string::npos)
    {
        pos = url.find('/', pos + 3);
        if (pos != std::string::npos)
            return url.substr(pos);
    }
    return url;
}


std::string ServerInit::handleRedirections(Server &server, HttpRequestParser &parser)
{
    std::string externalRedirect;
    std::string path = parser.getPath();
    if (!path.empty() && path[path.size() - 1] == '/' && path[path.size() - 2] == '/')
        return (ResolvePaths::normalizePath(path) + '/');
    if (parser.getMethod() == GET && !path.empty() && path[path.size() - 1] != '/' && path.find_last_of('/') != std::string::npos)
    {
        std::size_t lastSlashPos = path.find_last_of('/');
        std::string lastSegment = path.substr(lastSlashPos + 1);
        if (lastSegment.find('.') == std::string::npos && ResolvePaths::isLocation(server, path))
        {
            externalRedirect = path + "/";
            return externalRedirect;
        }
    }
    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it)
    {
        if (ResolvePaths::normalizePath(parser.getPath()) == ResolvePaths::normalizePath(it->getPath()) && !it->getRedirect().empty())
        {
            const std::string &redirectTarget = extractPathFromURL(it->getRedirect());
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
