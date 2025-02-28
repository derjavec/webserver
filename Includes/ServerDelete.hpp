#ifndef SERVERDELETE_HPP
#define SERVERDELETE_HPP

#include "Server.hpp"

class ServerDelete
{
    public:
        static void handleDeleteRequest(Server &server, int clientFd, HttpRequestParser &parser);
};

#endif