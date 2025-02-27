#include "Server.hpp"
#include "Webserver.hpp"

Server::Server(const ServerConfig& config): _serverFd(-1), _running(false), _root(config.getRoot())
{
    _upload            = config.getUpload();
    _serverName        = config.getServerName();
    _clientMaxBodySize = config.getClientMaxBodySize();
    _index             = config.getIndex();
    _autoindex         = config.isAutoindexEnabled();
    _errorPages        = config.getErrorPages();
    _locations         = config.getLocations();

    const std::vector<uint16_t>& ports = config.getPorts();
    if (ports.empty())
        throw ServerException("No ports specified in configuration.");
    ServerInit::handleMultiPorts(*this, ports);
    _serverFd = _listeningSockets[0];
    _port = ports[0];
    try
    {
        initEpoll();
    }
    catch (...)
    {
        stop();
        throw;
    }
    _running = true;
}

Server::~Server()
{
    stop();
}

Server::Server(const Server& obj)
{
    *this = obj;
}

Server& Server::operator=(const Server& obj)
{
    if (this != &obj)
    {
        _serverFd = obj._serverFd;
        _address = obj._address;
        _ip = obj._ip;
        _port = obj._port;
        _epollFd = obj._epollFd;
        _running = obj._running;
        _requestStartTime = obj._requestStartTime;
        _serverName = obj._serverName;
        _root = obj._root;
        _upload = obj._upload;
        _clientMaxBodySize = obj._clientMaxBodySize;
        _index = obj._index;
        _autoindex = obj._autoindex;
        _errorPages = obj._errorPages;
        _locations = obj._locations;
        _listeningSockets = obj._listeningSockets;
        _listeningAddresses = obj._listeningAddresses;
        _clientSessions = obj._clientSessions;
    }
    return *this;
}

void Server::initEpoll()
{
    _epollFd = epoll_create1(0);
    if (_epollFd == -1)
        throw ServerException("Failed to create epoll instance: " + std::string(strerror(errno)));

    for (size_t i = 0; i < _listeningSockets.size(); ++i)
    {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = _listeningSockets[i];
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _listeningSockets[i], &ev) == -1)
            throw ServerException("Failed to add listening socket to epoll: " + std::string(strerror(errno)));
    }
}

int Server::acceptClient(sockaddr_in &clientAddress, int listeningFd)
{
    socklen_t clientLen = sizeof(clientAddress);
    int clientFd = accept(listeningFd, (struct sockaddr *)&clientAddress, &clientLen);
    if (clientFd == -1)
        throw ServerException("Failed to accept client: " + std::string(strerror(errno)));
    else
    {
        int flags = fcntl(clientFd, F_GETFL, 0);
        if (flags == -1)
        {
            close(clientFd);
            throw ServerException("Failed to get client socket flags: " + std::string(strerror(errno)));
        }
        if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            close(clientFd);
            throw ServerException("Failed to set client socket non-blocking: " + std::string(strerror(errno)));
        }
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = clientFd;
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1)
        {
            close(clientFd);
            throw ServerException("Failed to add client to epoll: " + std::string(strerror(errno)));
        }
        std::cout << "New client connected (fd: " << clientFd << ")" << std::endl;
    }
    return clientFd;
}

void Server::answerClientEvent(int clientFd, std::vector<char>& clientBuffer)
{
    HttpRequestParser parser;
    try
    {
        int res = parser.parseRequest(clientBuffer);
        if (res !=0)
        {
            ServerErrors::handleErrors(*this, clientFd, res);
            return;
        }
        std::string filePath = parser.getPath();
        if (!ServerUtils::isValidURL(filePath))
        {
            std::cerr << "❌ Error: Invalid characters in request path: " << filePath << std::endl;
            ServerErrors::handleErrors(*this, clientFd, 400);
            return;
        }
        parser.setPath(filePath);
        std::string setCookieHeader = ServerInit::handleCookies(*this, parser, clientFd);
        std::string externalRedirect = ServerInit::handleRedirections(*this, parser);
        if (!externalRedirect.empty())
        {
            std::string response = "HTTP/1.1 301 Moved Permanently\r\n";
            response += setCookieHeader;
            response += "Location: " + externalRedirect + "\r\n";
            response += "Content-Length: 0\r\n\r\n";
            send(clientFd, response.c_str(), response.size(), 0);
            shutdown(clientFd, SHUT_WR);
            close(clientFd);
            return;
        }
        std::pair<std::string, std::string> fileInfo = parser.getContentType(filePath);
        std::string contentType = fileInfo.first;
        std::string fileCategory = fileInfo.second;
        
        if (parser.getMethod() == DELETE)
        {
            ServerDelete::handleDeleteRequest(*this, clientFd, parser);
            return;
        }
        filePath = ResolvePaths::resolveFilePath(*this, parser);
        std::cout << "filePath :" << filePath << std::endl;
        size_t dotPos = filePath.find_last_of(".");
        std::string fileExtension = (dotPos != std::string::npos) ? filePath.substr(dotPos + 1) : "";
        std::string scriptExecutor = ServerCGI::getScriptExecutor(fileExtension);
        if (fileCategory == "script")
        {
            ServerCGI::executeCGI(clientFd, filePath, parser.ToString(parser.getMethod()), parser.getBody(), scriptExecutor);
            return;
        }
        if (parser.getMethod() == POST)
        {
            ServerPost::handlePostRequest(*this, clientFd, parser, clientBuffer);
            return;
        }
        ServerGet::handleGetRequest(*this, clientFd, parser, contentType);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing request: " << e.what() << std::endl;
        std::string response = "HTTP/1.1 400 Bad Request\r\n";
        response += "Set-Cookie: session_id=" + _clientSessions[clientFd].sessionId + "; Path=/; HttpOnly\r\n";
        response += "Content-Length: 0\r\n\r\n";
        send(clientFd, response.c_str(), response.size(), 0);
        shutdown(clientFd, SHUT_WR);
        close(clientFd);
    }
}


void Server::handleRequest(int clientFd, std::map<int, std::vector<char> >& clientBuffers, std::map<int, int>& expectedContentLength)
{
    answerClientEvent(clientFd, clientBuffers[clientFd]);
    clientBuffers.erase(clientFd);
    expectedContentLength.erase(clientFd);
}

void Server::handleRecvErrors(int clientFd, ssize_t bytesReceived, std::map<int, std::vector<char> >& clientBuffers, std::map<int, int>& expectedContentLength)
{
    if (bytesReceived == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
    {
        std::cout << "📢 No more data available for now, waiting for next event." << std::endl;
        return;
    }
    else if (bytesReceived == 0) 
        std::cout << "❌ Client disconnected (fd: " << clientFd << ")" << std::endl;
    else 
        std::cerr << "❌ Error receiving data from client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
    close(clientFd);
    epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    clientBuffers.erase(clientFd);
    expectedContentLength.erase(clientFd);
}

void Server::handleClientEvent(int clientFd, uint32_t events)
{
    static std::map<int, std::vector<char> > clientBuffers;
    static std::map<int, int> expectedContentLength;
    if (events & EPOLLIN)
    {
        char buffer[4096] = {0};      
        ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            handleRecvErrors(clientFd, bytesReceived, clientBuffers, expectedContentLength);
            return;
        }
        clientBuffers[clientFd].insert(clientBuffers[clientFd].end(), buffer, buffer + bytesReceived);
        if (!ServerUtils::requestCompletelyRead(*this, clientFd, expectedContentLength, clientBuffers))
            return;
        handleRequest(clientFd, clientBuffers, expectedContentLength);
    }
    if (events & EPOLLOUT)
        ServerGet::handleClientWrite(*this, clientFd);
}

void Server::run()
{
    std::cout << "Server is running..." << std::endl;
    std::vector<int> fdsClients;
    const int MAX_EVENTS = min(512, _clientMaxBodySize / 2 + 10);
    struct epoll_event events[MAX_EVENTS];

    while (_running)
    {
        if (stopRequested == 1)
            break;
        int nfds = epoll_wait(_epollFd, events, MAX_EVENTS, 0);
        if (nfds == -1)
        {
            std::cerr << "Error in epoll_wait: " << strerror(errno) << std::endl;
            continue;
        }
        for (int i = 0; i < nfds; ++i)
        {
            int eventFd = events[i].data.fd;
            uint32_t eventType = events[i].events;
            bool isListening = false;
            for (size_t j = 0; j < _listeningSockets.size(); ++j)
            {
                if (eventFd == _listeningSockets[j])
                {
                    isListening = true;
                    break;
                }
            }
            if (isListening)
            {
                sockaddr_in clientAddress;
                try
                {
                    int clientFd = acceptClient(clientAddress, eventFd);
                    fdsClients.push_back(clientFd);
                }
                catch (const ServerException &e)
                {
                    std::cerr << "Error accepting client: " << e.what() << std::endl;
                    continue;
                }
            }
            else
                handleClientEvent(eventFd, eventType);
        }
    }
    for (size_t j = 0; j < _listeningSockets.size(); ++j)
    {
        if (_listeningSockets[j] != -1)
            close(_listeningSockets[j]);
    }
    for (std::vector<int>::const_iterator it = fdsClients.begin(); it != fdsClients.end(); ++it)
        close(*it);
    close(_epollFd);
}


void Server::stop()
{
    for (size_t i = 0; i < _listeningSockets.size(); ++i)
    {
        if (_listeningSockets[i] != -1)
            close(_listeningSockets[i]);
    }
    if (_serverFd != -1)
    {
        close(_serverFd);
        _serverFd = -1;
    }
    if (_epollFd != -1)
    {
        close(_epollFd);
        _epollFd = -1;
    }
    _running = false;
    std::cout << "Server stopped." << std::endl;
}

void Server::print() const
{
    std::cout << "=== Server Configuration ===" << std::endl;
    std::cout << "Server Name: " << (_serverName.empty() ? "Not set (default: localhost)" : _serverName) << std::endl;
    std::cout << "Port: " << _port << std::endl;
    std::cout << "Root Directory: " << (_root.empty() ? "Not set (default: ./)" : _root) << std::endl;

    if (_clientMaxBodySize != 0)
        std::cout << "Client Max Body Size: " << _clientMaxBodySize << " bytes" << std::endl;
    else
        std::cout << "Client Max Body Size: Not set (default: 1048576 bytes)" << std::endl;

    std::cout << "Default Index: " << (_index.empty() ? "Not set (default: index.html)" : _index) << std::endl;
    std::cout << "Autoindex: " << (_autoindex ? "Enabled" : "Disabled") << std::endl;

    std::cout << "Error Pages: " << std::endl;
    if (_errorPages.empty())
        std::cout << "  None configured." << std::endl;
    else
    {
        for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it)
            std::cout << "  " << it->first << ": " << it->second << std::endl;
    }

    std::cout << "Locations: " << std::endl;
    if (_locations.empty())
        std::cout << "  None configured." << std::endl;
    else
    {
        for (size_t i = 0; i < _locations.size(); ++i)
        {
            std::cout << "  Location " << i + 1 << ":" << std::endl;
            _locations[i].print();
        }
    }
    std::cout << "=============================" << std::endl;
}

ServerException::ServerException(const std::string &message)
    : _message(message)
{}

const char *ServerException::what() const throw()
{
    return _message.c_str();
}

ServerException::~ServerException() throw(){}

