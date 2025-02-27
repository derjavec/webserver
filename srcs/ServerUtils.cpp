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

std::string ServerUtils::resolveFilePath(Server &server, HttpRequestParser parser)
{
    std::string reqPath = parser.getPath();
    std::cout << "eq path "<< reqPath <<std::endl;
    size_t pos = reqPath.find('?');
    if (pos != std::string::npos)
        reqPath = reqPath.substr(0, pos);

    if (reqPath.length() > 1 && reqPath[reqPath.length() - 1] == '/')
        reqPath.erase(reqPath.length() - 1);
    LocationConfig *bestLocation = NULL;
    size_t bestMatchLength = 0;
    for (size_t i = 0; i < server._locations.size(); ++i)
    {
        std::string locPath = server._locations[i].getPath();
        if (locPath.length() > 1 && locPath[locPath.length() - 1] == '/')
            locPath.erase(locPath.length() - 1);

        if (reqPath.compare(0, locPath.length(), locPath) == 0 &&
            (reqPath.size() == locPath.size() || reqPath[locPath.size()] == '/'))
        {
            if (locPath.length() > bestMatchLength)
            {
                bestMatchLength = locPath.length();
                bestLocation = &server._locations[i];
            }
        }
    }
    std::string resolvedPath;
    if (bestLocation)
    {
        std::string relativePath = reqPath.substr(bestMatchLength);
        std::cout << "relative path "<< reqPath << " "<< relativePath<<std::endl;
        if (!relativePath.empty() && relativePath[0] == '/')
            relativePath.erase(0, 1);
        if (bestLocation->getAlias().empty() && bestLocation->getRoot().empty())
            resolvedPath = server._root + reqPath;
        else if (!bestLocation->getAlias().empty())
        {
            std::string alias = bestLocation->getAlias();
            if (alias[alias.length() - 1] != '/')
                alias += '/';
            resolvedPath = alias + relativePath;
        }
        else
        {
            std::string root = normalizePath(bestLocation->getRoot());
            std::string locationPath = normalizePath(bestLocation->getPath());
            std::string fileName;
            size_t lastSlash = relativePath.find_last_of("/");
            if (lastSlash != std::string::npos)
                fileName = relativePath.substr(lastSlash + 1);
            else 
                fileName = relativePath;
            if (fileName.find(".") == std::string::npos)
                fileName = "";
            std::string testPath = root + "/" + fileName;
            std::cout << "tesst path "<< testPath<<std::endl;
            bool isDir = isDirectory(testPath);
            if (!isDir && !std::ifstream(testPath.c_str()).good())
                root += locationPath;
            else if (isDir && isDirectory(root + locationPath))
                root += locationPath;
            resolvedPath = root;
            if (!relativePath.empty())
            {
                if (resolvedPath[resolvedPath.length() - 1] != '/')
                    resolvedPath += '/';
                resolvedPath += relativePath;
            }
        }
    } 
    else
    {
        std::string root = server._root;
        if (!root.empty() && root[root.length() - 1] != '/')
            root += '/';     
        if (reqPath.find('.') != std::string::npos && reqPath.find(".html") != std::string::npos)
            resolvedPath = root + "html/" + reqPath;
        else
            resolvedPath = root + reqPath;
    } 
    std::cout << "resolved path al final :"<< resolvedPath << std::endl;
    return urlDecode(resolvedPath);
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
        std::cerr << "No Content-Length found for client " << clientFd << std::endl;
    }
    return (0);
}

std::vector<char> ServerUtils::decodeChunked(const std::string &requestStr)
{
    std::vector<char> result;
    size_t headerEnd = requestStr.find("\r\n\r\n");

    if (headerEnd == std::string::npos)
        return result;

    std::string headers = requestStr.substr(0, headerEnd + 4);
    std::vector<char> headersVec(headers.begin(), headers.end());
    std::string bodyChunked = requestStr.substr(headerEnd + 4);
    std::vector<char> bodyDecoded;

    size_t chunkStart = 0;
    while (chunkStart < bodyChunked.size())
    {
        size_t chunkEnd = bodyChunked.find("\r\n", chunkStart);
        if (chunkEnd == std::string::npos)
            break;

        std::string chunkSizeHex = bodyChunked.substr(chunkStart, chunkEnd - chunkStart);
        int chunkSize = strtol(chunkSizeHex.c_str(), NULL, 16);
        if (chunkSize == 0)
            break;
        chunkStart = chunkEnd + 2;
        if (chunkStart + chunkSize > bodyChunked.size())
            break;
        bodyDecoded.insert(bodyDecoded.end(), bodyChunked.begin() + chunkStart, bodyChunked.begin() + chunkStart + chunkSize);
        chunkStart += chunkSize + 2;
    }
    result.insert(result.end(), headersVec.begin(), headersVec.end());
    result.insert(result.end(), bodyDecoded.begin(), bodyDecoded.end());
    return result;
}




bool ServerUtils::requestCompletelyRead(Server &server, int clientFd, std::map<int, int>& expectedContentLength, std::map<int, std::vector<char> >& clientBuffers)
{
    if (server._requestStartTime.find(clientFd) == server._requestStartTime.end())
        server._requestStartTime[clientFd] = clock();
    double elapsedTime = 0;
    std::string requestStr(clientBuffers[clientFd].begin(), clientBuffers[clientFd].end());
    size_t headerEndPos = requestStr.find("\r\n\r\n");
    if (headerEndPos == std::string::npos)
    {
        elapsedTime = (double)(clock() - server._requestStartTime[clientFd]) / CLOCKS_PER_SEC;
        if (elapsedTime > 5.0)
        {
            std::cerr << "❌ 408 Request Timeout: Header incomplete after 5 seconds." << std::endl;
            server._requestStartTime.erase(clientFd);
            ServerErrors::handleErrors(server, clientFd, 400);
            return false;
        }
        std::cerr << "🛑 Header incomplete, waiting for more data..." << std::endl;
        return false;
    }
    expectedContentLength[clientFd] = extractContentLength(clientFd, expectedContentLength, clientBuffers);
    size_t transferEncodingPos = requestStr.find("Transfer-Encoding: chunked");
    if (expectedContentLength[clientFd] > 0 && transferEncodingPos != std::string::npos)
    {
        std::cerr << "❌ 400 Bad Request: Content Length and Transfer Encoding found" << std::endl;
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
    
    if (transferEncodingPos != std::string::npos)
    {
        elapsedTime = (double)(clock() - server._requestStartTime[clientFd]) / CLOCKS_PER_SEC;
        size_t endChunk = requestStr.find("\r\n0\r\n\r\n");
        if (elapsedTime > 5.0)
        {
            std::cerr << "❌ 408 Request Timeout: Transfer-Encoding incomplete after 5 seconds." << std::endl;
            server._requestStartTime.erase(clientFd);
            ServerErrors::handleErrors(server, clientFd, 400);
            return false;
        }
        if (endChunk != std::string::npos)
        {
            server._requestStartTime.erase(clientFd);
            clientBuffers[clientFd] = ServerUtils::decodeChunked(requestStr);
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
    server._requestStartTime.erase(clientFd);

    return true;
}

std::string ServerUtils::urlDecode(const std::string& str)
{
    std::string decoded;
    char hexBuffer[3] = {0};

    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] == '+')
            decoded += ' ';
        else if (str[i] == '%' && i + 2 < str.length())
        {
            hexBuffer[0] = str[i + 1];
            hexBuffer[1] = str[i + 2];
            decoded += static_cast<char>(strtol(hexBuffer, NULL, 16));
            i += 2;
        }
        else
            decoded += str[i];
    }
    return decoded;
}


std::map<std::string, std::string> ServerUtils::parseURLEncoded(const std::string& data)
{
    std::map<std::string, std::string> formFields;
    std::istringstream stream(data);
    std::string pair;

    while (std::getline(stream, pair, '&'))
    {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos)
        {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            formFields[urlDecode(key)] = urlDecode(value);
        }
    }
    return formFields;
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

bool ServerUtils::findLocation(Server &server, const std::string &path)
{
    std::string normalizedPath = normalizePath(path);
    std::cout << "path en fl "<<normalizedPath << std::endl;
    const LocationConfig *bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it)
    {
        std::string locationPath = normalizePath(it->getPath());
        std::string fullLocationPath;
        if (it->getAlias().empty() && it->getRoot().empty())
            fullLocationPath = server._root + normalizedPath;
        else if (!it->getAlias().empty())
            fullLocationPath = normalizePath(it->getAlias());
        else
        {
            std::string root = normalizePath(it->getRoot());
            std::string fileName;
            size_t lastSlash = normalizedPath.find_last_of("/");
            if (lastSlash != std::string::npos)
                fileName = normalizedPath.substr(lastSlash + 1);
            else 
                fileName = normalizedPath;
            if (fileName.find(".") == std::string::npos)
                fileName = "";
            std::string testPath = root + "/" + fileName;
            bool isDir = isDirectory(testPath);
            std::cout << "tesst path "<< testPath<< " "<<fileName<<std::endl;
            if (!isDir && !std::ifstream(testPath.c_str()).good())
                fullLocationPath = root + locationPath;
            else if (isDir && isDirectory(root + locationPath))
                fullLocationPath = root + locationPath;
            else
                fullLocationPath = root;
            
        }
        std::cout << "full path en fl "<<fullLocationPath << std::endl;
        size_t found = normalizedPath.find(fullLocationPath);
        if (found == 0 || (found != std::string::npos && (normalizedPath.length() == fullLocationPath.length() || normalizedPath[found + fullLocationPath.length()] == '/')))
        {
            if (fullLocationPath.length() > bestMatchLength)
            {
                bestMatch = &(*it);
                bestMatchLength = fullLocationPath.length();
            }
        }
    }
    return bestMatch != NULL;
}

bool ServerUtils::findLocationConfig(Server &server, const std::string &path, std::string &locationIndex, bool &autoindexEnabled, std::string &filePath)
{
    std::string normalizedPath = normalizePath(path);
    if (normalizedPath.find(server._root) != 0)
        normalizedPath = server._root + normalizedPath;
    std::cout << "npath: "<< normalizedPath << std::endl;
    const LocationConfig *bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it)
    {
        std::string rootPath = normalizePath(it->getRoot().empty() ? server._root : it->getRoot());
        std::string locationPath = normalizePath(it->getPath());
        if (!locationPath.empty() && locationPath != "/")
        {
            if (isDirectory(server._root + locationPath) && (rootPath.length() < locationPath.length() ||
            rootPath.substr(rootPath.length() - locationPath.length()) != locationPath))
                rootPath += locationPath;
        }
        std::cout << "Comparando: " << normalizedPath << " vs " << rootPath << std::endl;
        if (normalizedPath.compare(0, rootPath.length(), rootPath) == 0 &&
            (normalizedPath.length() == rootPath.length() || normalizedPath[rootPath.length()] == '/'))
        {
            if (rootPath.length() > bestMatchLength)
            {
                bestMatch = &(*it);
                bestMatchLength = rootPath.length();
            }
        }
    }
    if (bestMatch)
    {
        std::cout<<"encuentra :"<<bestMatch->getPath()<<std::endl;
        locationIndex = bestMatch->getIndex();
        autoindexEnabled = bestMatch->isAutoindexEnabled();
        if (bestMatch->getAlias().empty() && bestMatch->getRoot().empty())
            filePath = normalizePath(server._root);
        else if (!bestMatch->getAlias().empty())
            filePath = normalizePath(bestMatch->getAlias());
        else
        {
            std::string root = normalizePath(bestMatch->getRoot());
            std::string locationPath = normalizePath(bestMatch->getPath());
            if (!locationIndex.empty())
            {
                std::string testPath = root + "/" + locationIndex;
                std::cout << "tesst path "<< testPath<<std::endl;
                if (!std::ifstream(testPath.c_str()).good())
                    root += locationPath;
                filePath = root;
            }
            else
                filePath = root;
            
        }
        return true;
    }
    return false;
}



std::string ServerUtils::extractPathFromURL(const std::string &url)
{
    size_t pos = url.find("://");
    if (pos != std::string::npos)
    {
        pos = url.find('/', pos + 3);
        if (pos != std::string::npos)
            return url.substr(pos);
    }
    return url;
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


std::string ServerUtils::normalizePath(const std::string &path)
{
    std::string normalized = path;

    while (normalized.length() > 1 && normalized[normalized.length() - 1] == '/')
        normalized.erase(normalized.length() - 1);
    return normalized;
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
