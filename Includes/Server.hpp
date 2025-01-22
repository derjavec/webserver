#ifndef SERVER_HPP
#define SERVER_HPP

# include "Webserver.hpp"
//# include "Client.hpp"
class Client;
class Server
{
    private :
        int _serverFd;
        sockaddr_in _address;
        std::string _ip;
        int _port;
        std::map<int, Client> _clients;
        bool _running;
        void initSocket();
        void bindSocket();
        void startListening();
    public :
        Server();
        Server(int port);
        ~Server();
        Server(const Server& obj);
        Server& operator=(const Server& obj);
        void run();
        void stop();
        int acceptClient(sockaddr_in &clientAddress);
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