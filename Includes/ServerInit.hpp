#ifndef SERVERINIT_HPP
#define SERVERINIT_HPP

#include "Server.hpp"

class ServerInit
{
    public:
        static std::string handleCookies(Server &server, HttpRequestParser &parser, int clientFd);
        static void handleMultiPorts(Server &server, const std::vector<uint16_t> &ports);
        static std::string handleRedirections(Server &server, HttpRequestParser &parser);
        static bool handleImplicitRedirections(Server &server, int clientFd, const std::string &filePath);
};

#endif
