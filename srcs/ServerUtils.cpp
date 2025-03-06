#include "ServerUtils.hpp"
#include "Server.hpp"

int min(int a, int b)
{
    return ((a < b) ? a : b);
}

bool directoryExists(const std::string &path)
{
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

int ServerUtils::extractContentLength(int clientFd, std::map<int, int>& expectedContentLength, std::map<int, std::vector<char> >& clientBuffers)
{

    if (expectedContentLength.find(clientFd) == expectedContentLength.end() || expectedContentLength[clientFd] == 0)
    {
        std::string header(clientBuffers[clientFd].begin(), clientBuffers[clientFd].end());
        size_t contentLengthPos = header.find("Content-Length: ");
        if (contentLengthPos != std::string::npos)
        {
            size_t endPos = header.find("\r\n", contentLengthPos);
            if (endPos != std::string::npos)
            {
                std::string contentLengthStr = header.substr(contentLengthPos + 16, endPos - (contentLengthPos + 16));             
                size_t contentLength = ServerUtils::stringToSizeT(contentLengthStr);
                expectedContentLength[clientFd] = contentLength;
                return contentLength;
            }
        }
    }
    return (0);
}

bool ServerUtils::isHeaderComplete(Server &server, int clientFd, const std::string &requestStr, std::string &method)
{
    if (server._requestStartTime.find(clientFd) == server._requestStartTime.end())
        server._requestStartTime[clientFd] = clock();

    std::istringstream requestStream(requestStr);
    requestStream >> method;

    size_t headerEndPos = requestStr.find("\r\n\r\n");
    if (headerEndPos == std::string::npos)
    {
        double elapsedTime = (double)(clock() - server._requestStartTime[clientFd]) / CLOCKS_PER_SEC;
        if (elapsedTime > 5.0)
        {
            std::cerr << "❌ 408 Request Timeout: Header incomplete after 5 seconds." << std::endl;
            server._requestStartTime.erase(clientFd);
            ServerErrors::handleErrors(server, clientFd, 408);
            return false;
        }
        std::cerr << "🛑 Header incomplete, waiting for more data..." << std::endl;
        return false;
    }
    return true;
}

bool ServerUtils::validateContentLengthAndEncoding(Server &server, int clientFd, std::string &requestStr, std::map<int, int> &expectedContentLength, std::map<int, std::vector<char> > &clientBuffers)
{
    expectedContentLength[clientFd] = extractContentLength(clientFd, expectedContentLength, clientBuffers);
    size_t transferEncodingPos = requestStr.find("Transfer-Encoding: chunked");

    if (expectedContentLength[clientFd] > 0 && transferEncodingPos != std::string::npos)
    {
        std::cerr << "❌ 400 Bad Request: Content-Length and Transfer-Encoding found" << std::endl;
        server._requestStartTime.erase(clientFd);
        ServerErrors::handleErrors(server, clientFd, 400);
        return false;
    }

    if (expectedContentLength[clientFd] > 0)
    {
        if (static_cast<int>(clientBuffers[clientFd].size()) >= expectedContentLength[clientFd])
        {
            server._requestStartTime.erase(clientFd);
            return true;
        }
        return false;
    }
    return true;
}

bool ServerUtils::handleTransferEncodingOrMultipart(Server &server, int clientFd, std::string &requestStr, std::map<int, std::vector<char> > &clientBuffers)
{
    size_t transferEncodingPos = requestStr.find("Transfer-Encoding: chunked");
    if (transferEncodingPos != std::string::npos)
    {
        double elapsedTime = (double)(clock() - server._requestStartTime[clientFd]) / CLOCKS_PER_SEC;
        size_t endChunk = requestStr.find("\r\n0\r\n\r\n");

        if (elapsedTime > 10.0)
        {
            std::cerr << "❌ 408 Request Timeout: Transfer-Encoding incomplete after 10 seconds." << std::endl;
            server._requestStartTime.erase(clientFd);
            ServerErrors::handleErrors(server, clientFd, 408);
            return false;
        }

        if (endChunk != std::string::npos)
        {
            server._requestStartTime.erase(clientFd);
            clientBuffers[clientFd] = ResolvePaths::decodeChunked(requestStr);
            size_t endHeader = requestStr.find("\r\n", transferEncodingPos);
            if (endHeader != std::string::npos)
                requestStr.erase(transferEncodingPos, endHeader - transferEncodingPos + 2);
            return true;
        }
        return false;
    }

    size_t contentTypePos = requestStr.find("Content-Type: multipart/form-data");
    if (contentTypePos != std::string::npos)
    {
        static std::map<int, std::string> boundaryMap;
        size_t boundaryPos = requestStr.find("boundary=", contentTypePos);

        if (boundaryPos != std::string::npos)
        {
            std::string extractedBoundary = requestStr.substr(boundaryPos + 9);
            size_t endBoundary = extractedBoundary.find_first_of("\r\n ");
            if (endBoundary != std::string::npos)
                extractedBoundary = extractedBoundary.substr(0, endBoundary);

            extractedBoundary = "--" + extractedBoundary;
            if (boundaryMap.find(clientFd) == boundaryMap.end())
                boundaryMap[clientFd] = extractedBoundary;
        }

        std::string endingBoundary = boundaryMap[clientFd] + "--";
        size_t secondBoundaryPos = requestStr.rfind(endingBoundary);
        if (secondBoundaryPos != std::string::npos)
        {
            server._requestStartTime.erase(clientFd);
            return true;
        }
        return false;
    }
    return true;
}

bool ServerUtils::requestCompletelyRead(Server &server, int clientFd, std::map<int, int> &expectedContentLength, std::map<int, std::vector<char> > &clientBuffers)
{
    std::string method;
    std::string requestStr(clientBuffers[clientFd].begin(), clientBuffers[clientFd].end());

    if (!isHeaderComplete(server, clientFd, requestStr, method))
        return false;

    if (!validateContentLengthAndEncoding(server, clientFd, requestStr, expectedContentLength, clientBuffers))
        return false;

    if (!handleTransferEncodingOrMultipart(server, clientFd, requestStr, clientBuffers))
        return false;

    server._requestStartTime.erase(clientFd);
    return true;
}

bool ServerUtils::isValidURL(const std::string &url)
{
    for (size_t i = 0; i < url.length(); i++)
    {
        char c = url[i];
        if (isalnum(c) || strchr("-._~:/?#[]@!$&'()*+,;=", c))
            continue;
        if (c == '%' && i + 2 < url.length() && isxdigit(url[i + 1]) && isxdigit(url[i + 2]))
        {
            i += 2;
            continue;
        }
        return false;
    }
    return true;
}

bool ServerUtils::isDirectory(const std::string &path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == 0)
    {
        return S_ISDIR(s.st_mode);
    }
    return false;
}

size_t ServerUtils::getFileSize(const std::string &filePath)
{
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == 0)
        return fileStat.st_size;
    return 0;
}

std::string ServerUtils::numberToString(int number)
{
    std::stringstream ss;
    ss << number;
    return ss.str();
}

size_t ServerUtils::stringToSizeT(const std::string& str)
{
    std::istringstream iss(str);
    size_t result = 0;
    iss >> result;
    if (iss.fail())
    {
        std::cerr << "Error: Could not convert string to size_t: " << str << std::endl;
        return 0;
    }
    return result;
}

std::string ServerUtils::generateSessionId()
{
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string sessionId;
    for (int i = 0; i < 16; ++i)
        sessionId += chars[rand() % chars.size()];
    return sessionId;
}

std::string ServerUtils::trim(std::string &str)
{
    if (str.empty())
        return str;
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// Nouvelles fonctions pour la gestion des en-têtes HTTP

std::string ServerUtils::buildStandardHeaders()
{
    std::string headers = "";
    headers += "Server: WebServer/1.0\r\n";
    headers += "Date: " + getCurrentDate() + "\r\n";
    return headers;
}

std::string ServerUtils::getCurrentDate()
{
    time_t now = time(0);
    struct tm* tm_info = gmtime(&now);
    char dateStr[100];
    strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    return std::string(dateStr);
}

std::string ServerUtils::getMimeType(const std::string& filePath)
{
    size_t dotPos = filePath.find_last_of(".");
    if (dotPos == std::string::npos)
        return "application/octet-stream";
    
    std::string extension = filePath.substr(dotPos + 1);
    
    // Convertir en minuscules
    std::string lowerExt = extension;
    for (size_t i = 0; i < lowerExt.size(); ++i) {
        lowerExt[i] = tolower(lowerExt[i]);
    }
    
    static std::map<std::string, std::string> mimeTypes;
    if (mimeTypes.empty()) {
        // HTML, CSS, JavaScript
        mimeTypes["html"] = "text/html";
        mimeTypes["htm"] = "text/html";
        mimeTypes["css"] = "text/css";
        mimeTypes["js"] = "application/javascript";
        
        // Images
        mimeTypes["jpg"] = "image/jpeg";
        mimeTypes["jpeg"] = "image/jpeg";
        mimeTypes["png"] = "image/png";
        mimeTypes["gif"] = "image/gif";
        mimeTypes["ico"] = "image/x-icon";
        
        // Autres types courants
        mimeTypes["pdf"] = "application/pdf";
        mimeTypes["zip"] = "application/zip";
        mimeTypes["json"] = "application/json";
        mimeTypes["xml"] = "application/xml";
        mimeTypes["txt"] = "text/plain";
    }
    
    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(lowerExt);
    if (it != mimeTypes.end())
        return it->second;
    
    return "application/octet-stream";
}
