#include "ServerFolders.hpp"

bool ServerFolders::handleFoldersRequests(Server &server, int clientFd, const std::string &path, const std::string &url)
{
    std::string locationIndex;
    bool autoindexEnabled = false;
    std::string filePath;
    if (!ResolvePaths::findLocationConfig(server, path, locationIndex, autoindexEnabled, filePath, url))
        return false;
    if (!locationIndex.empty())
    {
        filePath = filePath + "/"+ locationIndex;
        std::cout << filePath << std::endl;
        std::ifstream file(filePath.c_str());
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            file.close();
            if (content.empty())
            {
                ServerErrors::handleErrors(server, clientFd, 204);
                return true;
            }        
            std::string response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + ServerUtils::numberToString(content.size()) + "\r\n";
            response += "\r\n";
            response += content;
            send(clientFd, response.c_str(), response.size(), 0);
        }
        else
            ServerErrors::handleErrors(server, clientFd, 404);
        return true;
    }
    else
    {
        if (autoindexEnabled)
            handleAutoIndex(server, clientFd, autoindexEnabled, path);
        else
            ServerErrors::handleErrors(server, clientFd, 403);
        return true;
    }
}

void ServerFolders::handleAutoIndex(Server &server, int clientFd, bool autoindexEnabled, std::string path)
{
    if (autoindexEnabled)
    {
        std::string directoryListing = generateAutoindexPage(path);
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + ServerUtils::numberToString(directoryListing.size()) + "\r\n";
        response += "\r\n";
        response += directoryListing;
        send(clientFd, response.c_str(), response.size(), 0);
    }
    else
        ServerErrors::handleErrors(server, clientFd, 403);
    return ;
}

std::string ServerFolders::generateAutoindexPage(const std::string& directoryPath)
{
    DIR* dir = opendir(directoryPath.c_str());
    if (!dir)
        return "<html><body><h1>500 Internal Server Error</h1></body></html>";
    std::string page = "<html><body><h1>Index of " + directoryPath + "</h1><ul>";
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string fileName = entry->d_name;
        page += "<li><a href=\"" + fileName + "\">" + fileName + "</a></li>";
    }
    closedir(dir);
    page += "</ul></body></html>";
    return page;
}
