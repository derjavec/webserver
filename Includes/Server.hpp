#ifndef SERVER_HPP
#define SERVER_HPP

#include "Webserver.hpp"
#include "ServerConfig.hpp"
#include "HttpRequestParser.hpp"
#include "ServerUtils.hpp"
#include <vector>
#include <map>
#include <netinet/in.h>

#define LARGE_FILE_THRESHOLD 1048576

class Client;
class ServerUtils;

class Server
{
    private:
        std::vector<int>             _listeningSockets;
        std::vector<sockaddr_in>     _listeningAddresses;
        int                          _serverFd;
        sockaddr_in                  _address;
        std::string                  _ip;
        int                          _port;
        int                          _epollFd;
        bool                         _running;
        std::string                  _serverName;
        std::string                  _root;
        size_t                       _clientMaxBodySize;
        std::string                  _index;
        bool                         _autoindex;
        std::map<int, std::string>   _errorPages;
        std::vector<LocationConfig>  _locations;

        void initSocket();
        void bindSocket();
        void startListening();

        void initEpoll();

        friend class ServerUtils;

    public:
        Server(const ServerConfig& config);
        Server(int port);
        ~Server();
        Server(const Server& obj);
        Server& operator=(const Server& obj);

        void run();
        void stop();
        int acceptClient(sockaddr_in &clientAddress, int listeningFd);
        void handleClientEvent(int clientFd, uint32_t events);
        void answerClientEvent(int clientFd, ssize_t bytesReceived, char *buffer);
        void handleMultiPorts(const std::vector<uint16_t>& ports);
        void print() const;
};

class ServerException : public std::exception
{
    private:
        std::string _message;
    public:
        explicit ServerException(const std::string &message);
        ~ServerException() throw();
        virtual const char *what() const throw();
};

#endif

