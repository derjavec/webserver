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
        static int extractContentLength(int clientFd, std::map<int, int>& expectedContentLength, std::map<int, std::vector<char> >& clientBuffers);
        static bool isHeaderComplete(Server &server, int clientFd, const std::string &requestStr, std::string &method);
        static bool validateContentLengthAndEncoding(Server &server, int clientFd, std::string &requestStr, std::map<int, int> &expectedContentLength, std::map<int, std::vector<char> > &clientBuffers);
        static bool handleTransferEncodingOrMultipart(Server &server, int clientFd, std::string &requestStr, std::map<int, std::vector<char> > &clientBuffers);
        static bool requestCompletelyRead(Server &server, int clientFd, std::map<int, int>& expectedContentLength, std::map<int, std::vector<char> >& clientBuffers);
        static bool isValidURL(const std::string &url);
        static bool isDirectory(const std::string &path);
        static size_t getFileSize(const std::string &filePath);
        static std::string numberToString(int number);
        static size_t stringToSizeT(const std::string& str);
        static std::string generateSessionId(void);
        static std::string trim(std::string &str);
};

#endif
