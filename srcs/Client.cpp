#include "Client.hpp"

Client::Client():_clientFd(-1), _address(), _ip(""), _port(0) {}
Client::Client(int clientFd, const sockaddr_in& address): _clientFd(clientFd), _address(address)
{
    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &_address.sin_addr, ipBuffer, INET_ADDRSTRLEN);
    _ip = ipBuffer;
    _port = ntohs(_address.sin_port);
    std::cout << "Client connected: " << _ip << ":" << _port << std::endl;
}
Client::~Client()
{
    closeConnection();
}
Client::Client(const Client& obj)
{
    *this = obj;
}
Client& Client::operator=(const Client& obj)
{
    if (this != &obj)
    {
        _clientFd = -1;
        _address = obj._address;
        _ip = obj._ip;
        _port = obj._port;
    }
    return (*this);
}
std::string Client::getIp() const
{
    return (_ip);
}
int Client::getPort() const
{
    return (_port);
}
std::string Client::receiveMessage()
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesReceived = recv(_clientFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
            std::cerr << "Client disconnected: " << _ip << ":" << _port << std::endl;

        else
            std::cerr << "Error receiving message: " << strerror(errno) << std::endl;
        return ("");
    }
    return (std::string(buffer, bytesReceived));
}
ssize_t Client::sendMessage(const std::string& msg)
{
    ssize_t bytesSent = send(_clientFd, msg.c_str(), msg.length(), 0);
    if (bytesSent == -1)
        std::cerr << "Error sending message to client: " << std::endl;
    return (bytesSent);
}

void Client::closeConnection()
{
    if (_clientFd != -1)
    {
        close(_clientFd);
        std::cout << "Connection closed for client: " << _ip << ":" << _port << std::endl;
        _clientFd = -1;
    }
}