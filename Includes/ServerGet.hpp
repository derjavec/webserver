#ifndef SERVERGET_HPP
#define SERVERGET_HPP

#include "Server.hpp"
#include "SessionData.hpp"

struct ClientState 
{
    int fileFd;
    off_t offset;
    size_t totalSize;
};

class ServerGet
{
    public:
        static void handleClientWrite(Server &server, int clientFd);
        static void serveLargeFileAsync(Server &server, int clientFd, const std::string& filePath, const std::string &ContentType);
        static void handleRawPost(int clientFd);
        static void handleGetRequest(Server &server, int clientFd, HttpRequestParser &parser, const std::string &contentType, std::string &filePath);
};
void serveStaticFile(const std::map<int, SessionData> &clientSessions, Server &server, int clientFd, const std::string &filePath, const std::string &contentType);



#endif