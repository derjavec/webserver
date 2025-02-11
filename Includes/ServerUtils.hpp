#ifndef SERVERUTILS_HPP
#define SERVERUTILS_HPP

//# include "Server.hpp"
# include "Webserver.hpp"
# include "HttpRequestParser.hpp"
# include "LocationConfig.hpp"
# include <sys/stat.h>

bool directoryExists(const std::string &path);
class Server;

struct UploadData
{
    std::string boundary;
    std::string filename;
    std::string fileContent;
};

struct ClientState 
{
    int fileFd;
    off_t offset;
    size_t totalSize;
};

class ServerUtils
{      
    public:
        static std::string resolveFilePath(Server &server, HttpRequestParser parser);
        static UploadData parseFileUploadRequest(const std::string& request);
        static void handleFileUpload(Server &server, int clientFd, const std::string& request);
        static bool findLocationConfig(Server &server, const std::string &path, std::string &locationIndex, bool &autoindexEnabled, std::string &filePath);
        static std::string extractPathFromURL(const std::string &url);
        static void handleRedirections(Server &server, HttpRequestParser &parser, std::string &externalRedirect);
        static void handleFoldersRequests(Server &server, int clientFd, const std::string &path, const std::string &fileType);
        static void handleClientWrite(Server &server, int clientFd);
        static void serveLargeFileAsync(Server &server, int clientFd, const std::string& filePath, const std::string &ContentType);
        static void serveStaticFile(Server &server, int clientFd, const std::string &filePath, const std::string &contentType);
        static std::string getScriptExecutor(const std::string &fileExtension);
        static char **setupCGIEnvironment(const std::string &scriptPath, const std::string &method, const std::string &body);
        static void handleCGIChildProcess(int pipefd[2], const std::string &scriptExecutor, const std::string &scriptPath,const std::string &method, const std::string &body);
        static void handleCGIParentProcess(int clientFd, int pipefd[2], pid_t pid);
        static void executeCGI(int clientFd, const std::string &scriptPath, const std::string &method, const std::string &body, const std::string &scriptExecutor);
        static void handleAutoIndex(Server &server, int clientFd, bool autoindexEnabled, std::string path);
        static std::string generateAutoindexPage(Server &server, const std::string& directoryPath);
        static void handleDeleteRequest(Server &server, int clientFd, HttpRequestParser &parser);
        static size_t getFileSize(const std::string &filePath);
        static void sendGenericErrorResponse(int clientFd, int code);
        static void handleErrors(Server &server, int clientFd, int code);
        static std::string getStatusMessage(int code);
        static std::string numberToString(int number);
        static std::string normalizePath(const std::string &path);
};

#endif
