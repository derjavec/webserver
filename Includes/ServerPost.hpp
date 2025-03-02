#ifndef SERVERPOST_HPP
#define SERVERPOST_HPP

#include "Server.hpp"

struct UploadData
{
    std::string boundary;
    std::string filename;
    std::string fileContent;
};

class ServerPost
{
    public:
        static bool getFormFilePath(Server &server, const std::string& formFileName, std::string& formFilePath);
        static void handleFileUpload(Server &server, int clientFd, const std::vector<char>& requestBuffer);
        static std::string getRootFromForm(Server &server);
        static void handleFormSubmission(Server &server, int clientFd, std::vector<char>& clientBuffer, const std::string &sessionId, const std::string &path);
        static std::map<std::string, std::string> getFormFields(Server &server, int clientFd, std::vector<char>& clientBuffer, std::string &formFilePath);
        static void handleFormLogin(Server &server, int clientFd, std::vector<char>& clientBuffer);
        static bool checkClientSession(Server &server, int clientFd, std::vector<char>& clientBuffer, const std::string &sessionId);
        static void handlePostRequest(Server &server, int clientFd, HttpRequestParser& parser, std::vector<char>& clientBuffer);
};
UploadData parseFileUploadRequest(const std::vector<char>& requestBuffer);
void handleBigFileUpload(Server &server, int clientFd, UploadData data, std::string filePath);
void handleSmallFileUpload(Server &server, int clientFd, UploadData data, std::string filePath);
bool writeFormData(const std::string& formFilePath, const std::map<std::string, std::string>& formFields, std::string sessionId);
bool extractFormData(const std::vector<char>& clientBuffer, std::string& formData);
std::string verifyLogin(const std::string& filePath, const std::map<std::string, std::string>& formFields);
std::string findUsernameBySessionId(const std::string &filePath, const std::string &sessionId);




#endif