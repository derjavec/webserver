#ifndef SERVERUTILS_HPP
#define SERVERUTILS_HPP

//# include "Server.hpp"
# include "Webserver.hpp"
# include "HttpRequestParser.hpp"
# include "LocationConfig.hpp"
# include <sys/stat.h>

int min(int a, int b);
bool directoryExists(const std::string &path);
class Server;

class ServerUtils
{      
    public:
        static std::string resolveFilePath(Server &server, HttpRequestParser parser);
        static std::map<std::string, std::string> parseURLEncoded(const std::string& data);
        static std::string urlDecode(const std::string& str);
        static int extractContentLength(int clientFd, std::map<int, int>& expectedContentLength, std::map<int, std::vector<char> >& clientBuffers);
        static std::vector<char> decodeChunked(const std::string &requestStr);
        static bool requestCompletelyRead(Server &server, int clientFd, std::map<int, int>& expectedContentLength, std::map<int, std::vector<char> >& clientBuffers);
        static bool isValidURL(const std::string &url);
        static bool isDirectory(const std::string &path);
        static bool findLocation(Server &server, const std::string &path);
        static bool findLocationConfig(Server &server, const std::string &path, std::string &locationIndex, bool &autoindexEnabled, std::string &filePath);
        static std::string extractPathFromURL(const std::string &url);
        static size_t getFileSize(const std::string &filePath);
        static std::string numberToString(int number);
        static size_t stringToSizeT(const std::string& str);
        static std::string normalizePath(const std::string &path);
        static std::string generateSessionId(void);
        static std::string trim(std::string &str);
};

#endif
