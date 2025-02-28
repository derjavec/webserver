#ifndef SERVERFOLDERS_HPP
#define SERVERFOLDERS_HPP

#include "Server.hpp"

class ServerFolders
{
    public:
        static bool handleFoldersRequests(Server &server, int clientFd, std::string path, std::string &url);
        static void handleAutoIndex(Server &server, int clientFd, bool autoindexEnabled, std::string path);
        static std::string generateAutoindexPage(const std::string& directoryPath);
};

#endif