#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Webserver.hpp"

class Client
{
    private :
        int  _clientFd;
        sockaddr_in _address;
        std::string _ip;
        int _port;
    public :
        Client();
        Client(int clientFd, const sockaddr_in& address);
        ~Client();
        Client(const Client& obj);
        Client& operator=(const Client& obj);
        std::string getIp() const;
        int getPort() const;
        ssize_t sendMessage(const std::string& msg);
        std::string receiveMessage();
        void closeConnection();
};
#endif