#include "Server.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <sys/epoll.h>
#include <errno.h>
#include <sstream>

Server::Server(const ServerConfig& config): _serverFd(-1), _running(false), _root(config.getRoot())
{
    _serverName        = config.getServerName();
    _clientMaxBodySize = config.getClientMaxBodySize();
    _index             = config.getIndex();
    _autoindex         = config.isAutoindexEnabled();
    _errorPages        = config.getErrorPages();
    _locations         = config.getLocations();

    const std::vector<uint16_t>& ports = config.getPorts();
    if (ports.empty())
        throw ServerException("No ports specified in configuration.");
    handleMultiPorts(ports);
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
        _serverName = obj._serverName;
        _root = obj._root;
        _clientMaxBodySize = obj._clientMaxBodySize;
        _index = obj._index;
        _autoindex = obj._autoindex;
        _errorPages = obj._errorPages;
        _locations = obj._locations;
        _listeningSockets = obj._listeningSockets;
        _listeningAddresses = obj._listeningAddresses;
    }
    return *this;
}


void Server ::handleMultiPorts(const std::vector<uint16_t>& ports)
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
        _listeningSockets.push_back(sockfd);
        _listeningAddresses.push_back(addr);
        std::cout << "Server listening on port " << ports[i] << std::endl;
    }
}

void Server::initSocket() {}
void Server::bindSocket() {}
void Server::startListening() {}

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
    std::cout << "Epoll instance created and all listening sockets registered." << std::endl;
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
        ev.events = EPOLLIN | EPOLLET;
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


void Server::answerClientEvent(int clientFd, ssize_t bytesReceived, char *buffer)
{
    std::string rawRequest(buffer, bytesReceived);
    HttpRequestParser parser;
    try
    {
        parser.parseRequest(rawRequest);
        std::cout << "Request parsed successfully!" << std::endl;
        std::cout << "Method: " << parser.getMethod() << std::endl;
        std::string externalRedirect;
        ServerUtils::handleRedirections(*this, parser, externalRedirect);
        if (!externalRedirect.empty())
        {
            std::string response = "HTTP/1.1 301 Moved Permanently\r\n"
                                   "Location: " + externalRedirect + "\r\n"
                                   "Content-Length: 0\r\n\r\n";
            send(clientFd, response.c_str(), response.size(), 0);
            shutdown(clientFd, SHUT_WR);
            close(clientFd);
            return;
        }
        if (parser.getMethod() == DELETE)
        {
            ServerUtils::handleDeleteRequest(*this, clientFd, parser);
            return;
        }
        if (parser.ToString(parser.getMethod()) == "POST" &&
            parser.getPath().find("/upload") == 0)
        {
            ServerUtils::handleFileUpload(*this, clientFd, rawRequest);
            return;
        }
        std::string filePath = ServerUtils::resolveFilePath(*this, parser);
        std::cout << "Path: " << filePath << std::endl;
        size_t fileSize = ServerUtils::getFileSize(filePath);
        std::pair<std::string, std::string> fileInfo = parser.getType();
        std::cout << "fileinfo: " << fileInfo.first << " " << fileInfo.second << std::endl;
        std::string contentType = fileInfo.first;
        std::string fileCategory = fileInfo.second;
        size_t dotPos = filePath.find_last_of(".");
        std::string fileExtension = (dotPos != std::string::npos) ? filePath.substr(dotPos + 1) : "";
        std::string scriptExecutor = ServerUtils::getScriptExecutor(fileExtension);
        //std::cout << "Script Executor: " << scriptExecutor << std::endl;
        if (fileCategory == "script")
        {
            ServerUtils::executeCGI(clientFd, filePath, parser.ToString(parser.getMethod()),
                                    parser.getBody(), scriptExecutor);
            return;
        }
        if (parser.getPath() == "/")
            filePath = _root + "/" + _index;
        else if (!filePath.empty() && filePath[filePath.size() - 1] == '/')
        {
            ServerUtils::handleFoldersRequests(*this, clientFd, parser.getPath(), contentType);
            return;
        }
        else if (fileSize > LARGE_FILE_THRESHOLD)
        {
            std::cout << "Serving large file: " << fileSize << " bytes (Non-blocking)" << std::endl;
            ServerUtils::serveLargeFileAsync(*this, clientFd, filePath, contentType);
            return ;
        }
        ServerUtils::serveStaticFile(*this, clientFd, filePath, contentType);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing request: " << e.what() << std::endl;
        std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(clientFd, response.c_str(), response.size(), 0);
        shutdown(clientFd, SHUT_WR);
        close(clientFd);
    }
}

void Server::handleClientEvent(int clientFd, uint32_t events)
{
    char buffer[4096] = {0};
    ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);

    if (events & EPOLLIN)
    {
        if (bytesReceived <= 0)
        {
            if (bytesReceived == 0)
                std::cout << "Client disconnected (fd: " << clientFd << ")" << std::endl;
            else
                std::cerr << "Error receiving message from client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
            close(clientFd);
            epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        }
        else
            answerClientEvent(clientFd, bytesReceived, buffer);
    }
    if (events & EPOLLOUT)
    {
        std::cout << "🔹 EPOLLOUT triggered for client " << clientFd << std::endl;
        ServerUtils::handleClientWrite(*this, clientFd);
    }
}


void Server::run()
{
    std::cout << "Server is running..." << std::endl;
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while (_running)
    {
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
                    acceptClient(clientAddress, eventFd);
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

