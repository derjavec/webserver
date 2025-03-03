#ifndef SERVERCGI_HPP
#define SERVERCGI_HPP

#include "Server.hpp"

class ServerCGI
{
    public:
        static std::string getScriptExecutor(const std::string &fileExtension);
        static char **setupCGIEnvironment(const std::string &scriptPath, const std::string &method, const std::string &body);
        static void handleCGIChildProcess(int pipefd[2], const std::string &scriptExecutor, const std::string &scriptPath,const std::string &method, const std::string &body);
        static void handleCGIParentProcess(Server &server, int clientFd, int pipefd[2], pid_t pid);
        static void executeCGI(Server &server, int clientFd, std::string &filePath, HttpRequestParser &parser);
        static bool hasCGIPassInLoc(Server &server, std::string url);
        static bool getCGIdataInLoc(Server &server, std::string url, std::vector<std::string>& cgiPaths, std::vector<std::string>& cgiExt);
};

#endif