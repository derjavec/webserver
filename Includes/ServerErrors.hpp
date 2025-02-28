#ifndef SERVERERRORS_HPP
#define SERVERERRORS_HPP

#include "Server.hpp"

class ServerErrors
{
    public:
    static void sendGenericErrorResponse(int clientFd, int code);
    static void handleErrors(Server &server, int clientFd, int code);
    static std::string getStatusMessage(int code);
};

#endif