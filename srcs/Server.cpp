#include "Server.hpp"
#include "Client.hpp"

Server::Server(){}
Server::Server(int port): _serverFd(-1), _port(port), _running(false)
{
    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_port);
    _address.sin_addr.s_addr = INADDR_ANY;
    try
    {
        initSocket();
        bindSocket();
        startListening();
    }
    catch (...)
    {
        stop();
        throw;
    }
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

int Server::acceptClient(sockaddr_in &clientAddress)
{
    socklen_t clientLen = sizeof(clientAddress);
    int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddress, &clientLen);
    if (clientFd == -1)
        throw ServerException("Failed to accept client: " + std::string(strerror(errno)));
    else
    {
        _clients[clientFd] = Client(clientFd, clientAddress);
        std::cout << "New client connected (fd: " << clientFd << ")" << std::endl;
    }
    return (clientFd);
}

void Server::run()
{

    std::cout << "Server is running..." << std::endl;
    fd_set readFds;
    int maxFd = _serverFd;
    while (_running)
    {
        FD_ZERO(&readFds);
        FD_SET(_serverFd, &readFds);
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            FD_SET(it->first, &readFds);
            maxFd = std::max(maxFd, it->first);
        }
        int activity = select(maxFd + 1, &readFds, NULL, NULL, NULL);
        if (activity < 0)
        {
            std::cerr << "Error in select: " << strerror(errno) << std::endl;
            continue;
        }
        if (FD_ISSET(_serverFd, &readFds))
        {
            sockaddr_in clientAddress;
            int clientFd = acceptClient(clientAddress);
             _clients[clientFd] = Client(clientFd, clientAddress);
        }
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end();)
        {
            int clientFd = it->first;
            Client &client = it->second;
            if (FD_ISSET(clientFd, &readFds)) 
            {
                std::string message = client.receiveMessage();
                if (message.empty())
                {
                    close(clientFd);
                    it = _clients.erase(it);
                    continue;
                }
                std::cout << "Client " << clientFd << " says: " << message << std::endl;
                client.sendMessage("Message received: " + message);
            }
            ++it;
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

ServerException::ServerException(const std::string &message): _message(message){}
const char *ServerException::what() const throw()
{
    return _message.c_str();
}

 ServerException::~ServerException() throw(){}