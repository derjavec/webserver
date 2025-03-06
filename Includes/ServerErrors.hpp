#ifndef SERVERERRORS_HPP
#define SERVERERRORS_HPP

#include "Server.hpp"

class ServerErrors
{
    public:
    static void sendGenericErrorResponse(int clientFd, int code, const std::vector<std::string>& allowedMethods = std::vector<std::string>());
    static void handleErrors(Server &server, int clientFd, int code, const std::vector<std::string>& allowedMethods = std::vector<std::string>());
    static std::string getStatusMessage(int code);
};

#endif