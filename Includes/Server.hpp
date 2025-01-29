#ifndef SERVER_HPP
#define SERVER_HPP

# include "Webserver.hpp"
# include "ServerConfig.hpp"
# include "HttpRequestParser.hpp"

class Client;
class Server
{
    private :
        int _serverFd;
        sockaddr_in _address;
        std::string _ip;
        int _port;
        int _epollFd;
        bool _running;
        std::string _serverName;
        std::string _root;
        size_t _clientMaxBodySize;
        std::string _index;
        bool _autoindex;
        std::map<int, std::string> _errorPages;
        std::vector<LocationConfig> _locations;

        void initSocket();
        void bindSocket();
        void startListening();
        void initEpoll();
    public :
        Server(const ServerConfig& config);
        Server(int port);
        ~Server();
        Server(const Server& obj);
        Server& operator=(const Server& obj);
        void run();
        void stop();
        int acceptClient(sockaddr_in &clientAddress);
        void handleClientEvent(int clientFd);
        void answerClientEvent(int clientFd, ssize_t bytesReceived, char *buffer);
        int handleFoldersRequests(int clientFd, std::string path, std::string filePath);
        void handleAutoIndex(int clientFd, bool autoindexEnabled, std::string path);
        std::string generateAutoindexPage(const std::string& directoryPath);
        void handleErrors(int clientFd, int code);
        std::string getStatusMessage(int code);
        std::string numberToString(int number);
        std::string normalizePath(const std::string &path);
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