#ifndef SERVERINIT_HPP
#define SERVERINIT_HPP

#include "Server.hpp"

class ServerInit
{
    public:
        static std::string handleCookies(Server &server, HttpRequestParser &parser, int clientFd);
        static void handleMultiPorts(Server &server);
        static std::string handleRedirections(Server &server, HttpRequestParser &parser);
};

std::string extractPathFromURL(const std::string &url);
#endif
