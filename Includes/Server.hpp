#ifndef SERVER_HPP
#define SERVER_HPP

#include "Webserver.hpp"
#include "ServerConfig.hpp"
#include "HttpRequestParser.hpp"
#include "FDStreamBuf.hpp"
#include "ServerUtils.hpp"
#include "ServerInit.hpp"
#include "ServerPost.hpp"
#include "ServerCGI.hpp"
#include "ServerGet.hpp"
#include "ServerFolders.hpp"
#include "ServerDelete.hpp"
#include "ServerErrors.hpp"
#include "SessionData.hpp"
#include "ResolvePaths.hpp"
#include <vector>
#include <map>
#include <netinet/in.h>

class Client;
class ServerUtils;

class Server
{
    private:
        std::vector<int>             _listeningSockets;
        std::vector<sockaddr_in>     _listeningAddresses;
        std::map<int, SessionData>   _clientSessions;
        int                          _serverFd;
        sockaddr_in                  _address;
        std::string                  _ip;
        int                          _port;
        int                          _epollFd;
        bool                         _running;
        std::map<int, clock_t>       _requestStartTime;
        std::string                  _serverName;
        std::string                  _root;
        std::string                  _upload;
        size_t                       _clientMaxBodySize;
        std::string                  _index;
        bool                         _autoindex;
        std::map<int, std::string>   _errorPages;
        std::vector<LocationConfig>  _locations;
        std::map <std::string, std::string> _sessionStore;

        void initEpoll();

        friend class ServerUtils;
        friend class ServerInit;
        friend class ServerPost;
        friend class ServerGet;
        friend class ServerFolders;
        friend class ServerErrors;
        friend class ResolvePaths;

    public:
        Server(const ServerConfig& config);
      //  Server(int port);
        ~Server();
        Server(const Server& obj);
        Server& operator=(const Server& obj);

        void run();
        void stop();
        int  acceptClient(sockaddr_in &clientAddress, int listeningFd);
        void handleRequest(int clientFd, std::map<int, std::vector<char> >& clientBuffers, std::map<int, int>& expectedContentLength);
        void handleRecvErrors(int clientFd, ssize_t bytesReceived, std::map<int, std::vector<char> >& clientBuffers, std::map<int, int>& expectedContentLength);
        void handleClientEvent(int clientFd, uint32_t events);
        void answerClientEvent(int clientFd, std::vector<char>& clientBuffer);
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

