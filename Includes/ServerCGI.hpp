#ifndef SERVERCGI_HPP
#define SERVERCGI_HPP

#include "Server.hpp"

class ServerCGI
{
    public:
        static std::string getScriptExecutor(const std::string &fileExtension);
        static char **setupCGIEnvironment(const std::string &scriptPath, const std::string &method, const std::string &body);
        static void handleCGIChildProcess(int pipefd[2], const std::string &scriptExecutor, const std::string &scriptPath,const std::string &method, const std::string &body);
        static void handleCGIParentProcess(int clientFd, int pipefd[2], pid_t pid);
        static void executeCGI(int clientFd, const std::string &scriptPath, const std::string &method, const std::string &body, const std::string &scriptExecutor); 
};

#endif