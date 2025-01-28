#include "Server.hpp"
#include "Client.hpp"

Server::Server(const ServerConfig& config): _serverFd(-1), _port(config.getPort()), _running(false) , _root(config.getRoot())
{
    _serverName = config.getServerName();
    _clientMaxBodySize = config.getClientMaxBodySize();
    _index = config.getIndex();
    _autoindex = config.isAutoindexEnabled();
    _errorPages = config.getErrorPages();
    _locations = config.getLocations();
    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_port);
    _address.sin_addr.s_addr = INADDR_ANY;
    try
    {
        initSocket();
        bindSocket();
        startListening();
        initEpoll();
    }
    catch (...)
    {
        stop();
        throw;
    }

    //print();
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
        _running = obj._running;
    }
    return (*this);
}

void Server::initSocket()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1)
        throw ServerException("Failed to create socket: " + std::string(strerror(errno)));
    int flags = fcntl(_serverFd, F_GETFL, 0);
    if (flags == -1)
    {
        throw ServerException("Failed to get socket flags: " + std::string(strerror(errno)));
    }
    if (fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        throw ServerException("Failed to set socket non-blocking: " + std::string(strerror(errno)));
    }
    std::cout << "Socket created successfully." << std::endl;
}

void Server::bindSocket()
{
    if (bind(_serverFd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
        throw ServerException("Failed to bind socket: " + std::string(strerror(errno)));
    std::cout << "Socket bound to port " << _port << std::endl;
}

void Server::startListening()
{
    if (listen(_serverFd, SOMAXCONN) < 0)
        throw ServerException("Failed to start listening: " + std::string(strerror(errno)));
    _running = true;
    std::cout << "Server is listening on port " << _port << std::endl;
}

void Server::initEpoll()
{
     _epollFd = epoll_create1(0);
     if (_epollFd == -1)
    {
        throw ServerException("Failed to create epoll instance: " + std::string(strerror(errno)));
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = _serverFd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverFd, &ev) == -1)
    {
        throw ServerException("Failed to add server socket to epoll: " + std::string(strerror(errno)));
    }
    std::cout << "Epoll instance created and server socket registered." << std::endl;
}

int Server::acceptClient(sockaddr_in &clientAddress)
{
    socklen_t clientLen = sizeof(clientAddress);
    int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddress, &clientLen);
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
    return (clientFd);
}

// void Server::processRequest(int clientFd, const std::string& message)
// {
//     std::istringstream requestStream(message);
//     std::string method, path, version;
//     requestStream >> method >> path >> version;
//     if (method != "GET")
//     {
//         std::map<int, std::string>::iterator it = _errorPages.find(405);
//         if (it != _errorPages.end())
//         {
//             const std::string& errorPage = it->second;
//             std::cout << "Error page for 405: " << errorPage << std::endl;
//         } 
//         else 
//             std::cerr << "Error page for 405 not found in the map." << std::endl;

//     }
//     if (path == "/")
//         path = _root + _index;
//     std::ifstream file(path.c_str());
//     if (!file.is_open())
//     {
//         std::map<int, std::string>::iterator it = _errorPages.find(404);
//         if (it != _errorPages.end())
//             sendHttpResponse(clientFd, "HTTP/1.1 404 Not Found\r\n\r\n" + it->second);
//         else
//             sendHttpResponse(clientFd, "HTTP/1.1 404 Not Found\r\n\r\n");
//         return;
//     }
//     std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//     file.close();
//     std::ostringstream response;
//     response << "HTTP/1.1 200 OK\r\n"
//              << "Content-Type: text/html\r\n"
//              << "Content-Length: " << content.size() << "\r\n"
//              << "\r\n" 
//              << content;
//     sendHttpResponse(clientFd, response.str());
// }

// void Server::sendHttpResponse(int clientFd, const std::string &response)
// {
//     ssize_t bytesSent = send(clientFd, response.c_str(), response.size(), 0);
//     if (bytesSent == -1) {
//         std::cerr << "Error sending response to client (fd: " << clientFd
//                   << "): " << strerror(errno) << std::endl;
//     } else {
//         std::cout << "Response sent to client (fd: " << clientFd
//                   << ") [" << bytesSent << " bytes]" << std::endl;
//     }
// }


void Server::handleClientEvent(int clientFd)
{
    char buffer[4096] = {0};
    ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);

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
    {
        std::string rawRequest(buffer, bytesReceived);
        try
        {
            HttpRequestParser parser;
            parser.parseRequest(rawRequest);
            std::cout << "Request parsed successfully!" << std::endl;
            std::cout << "Method: " << parser.getMethod() << std::endl;
            std::cout << "Path: " << parser.getPath() << std::endl;
            std::string filePath = _root + "/" + _index;
            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
            if (send(clientFd, response.c_str(), response.size(), 0) == -1)
                std::cerr << "Error sending message to client (fd: " << clientFd << "): " << strerror(errno) << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing request: " << e.what() << std::endl;
            std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            send(clientFd, response.c_str(), response.size(), 0);
        }
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
            if (events[i].data.fd == _serverFd)
            {
                sockaddr_in clientAddress;
                try
                {
                     acceptClient(clientAddress);   
                }   
                catch (const ServerException &e)
                {
                    std::cerr << "Error accepting client: " << e.what() << std::endl;
                    continue;
                }
            }
            else
                handleClientEvent(events[i].data.fd);
        }
    }

}

void Server::stop()
{
    if (_serverFd != -1)
    {
        close(_serverFd);
        _serverFd = -1;
        _running = false;
        std::cout << "Server stopped." << std::endl;
    }
}

void Server::print() const
{
    std::cout << "=== Server Configuration ===" << std::endl;
    std::cout << "Server Name: " << (_serverName.empty() ? "Not set (default: localhost)" : _serverName) << std::endl;
    std::cout << "Port: " << _port << std::endl;
    std::cout << "Root Directory: " << (_root.empty() ? "Not set (default: ./)" : _root) << std::endl;

    if (_clientMaxBodySize != 0) {
        std::cout << "Client Max Body Size: " << _clientMaxBodySize << " bytes" << std::endl;
    } else {
        std::cout << "Client Max Body Size: Not set (default: 1048576 bytes)" << std::endl;
    }

    std::cout << "Default Index: " << (_index.empty() ? "Not set (default: index.html)" : _index) << std::endl;
    std::cout << "Autoindex: " << (_autoindex ? "Enabled" : "Disabled") << std::endl;

    std::cout << "Error Pages: " << std::endl;
    if (_errorPages.empty()) {
        std::cout << "  None configured." << std::endl;
    } else {
        for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
    }

    std::cout << "Locations: " << std::endl;
    if (_locations.empty()) {
        std::cout << "  None configured." << std::endl;
    } else {
        for (size_t i = 0; i < _locations.size(); ++i) {
            std::cout << "  Location " << i + 1 << ":" << std::endl;
            _locations[i].print();
        }
    }
    std::cout << "=============================" << std::endl;
}




ServerException::ServerException(const std::string &message): _message(message){}
const char *ServerException::what() const throw()
{
    return _message.c_str();
}

 ServerException::~ServerException() throw(){}