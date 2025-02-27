#include "ResolvePaths.hpp"

const LocationConfig* ResolvePaths::findBestLocation(Server &server, const std::string &path, size_t &bestMatchLength)
{
    const LocationConfig* bestMatch = NULL;
    std::string normalizedPath = normalizePath(path);

    for (size_t i = 0; i < server._locations.size(); ++i)
    {
        std::string locPath = normalizePath(server._locations[i].getPath());
        if (normalizedPath.compare(0, locPath.length(), locPath) == 0 &&
            (normalizedPath.size() == locPath.size() || normalizedPath[locPath.size()] == '/'))
        {
            if (locPath.length() > bestMatchLength)
            {
                bestMatchLength = locPath.length();
                bestMatch = &server._locations[i];
            }
        }
    }
    return bestMatch;
}

std::string ResolvePaths::computeResolvedPath(const Server &server, const LocationConfig *bestLocation, const std::string &reqPath, std::string &relativePath)
{
    std::string resolvedPath;
    
    if (!relativePath.empty() && relativePath[0] == '/')
        relativePath.erase(0, 1);

    if (bestLocation->getAlias().empty() && bestLocation->getRoot().empty())
        resolvedPath = server._root + reqPath;
    else if (!bestLocation->getAlias().empty())
    {
        std::string alias = normalizePath(bestLocation->getAlias());
        resolvedPath = alias + "/" + relativePath;
    }
    else
    {
        std::string root = normalizePath(bestLocation->getRoot());
        std::string locationPath = normalizePath(bestLocation->getPath());
        resolvedPath = buildFullLocationPath(relativePath, root, locationPath);
        if (!relativePath.empty())
        {
            if (!resolvedPath.empty() && resolvedPath[resolvedPath.size() - 1] != '/')
                resolvedPath += '/';
            resolvedPath += relativePath;
        }
    }
    return resolvedPath;
}

std::string ResolvePaths::resolveFilePath(Server &server, HttpRequestParser parser)
{
    std::string reqPath = doubleNormalizePath(parser.getPath());
    size_t bestMatchLength = 0;
    const LocationConfig* bestLocation = findBestLocation(server, reqPath, bestMatchLength);
    std::string resolvedPath;
   
    if (bestLocation)
    {
        std::string relativePath = reqPath.substr(bestMatchLength);
        resolvedPath = computeResolvedPath(server, bestLocation, reqPath, relativePath);
    } 
    else
    {
        std::string root = normalizePath(server._root);   
        if (reqPath.find('.') != std::string::npos && reqPath.find(".html") != std::string::npos)
            resolvedPath = root + "/html" + reqPath;
        else
            resolvedPath = root + reqPath;
    } 
    return urlDecode(resolvedPath);
}

const LocationConfig* ResolvePaths::findBestLocationMatchForConfig(Server &server, const std::string &normalizedPath, const std::string &url)
{
    const LocationConfig *bestMatch = NULL;
    size_t bestMatchLength = 0;
    for (std::vector<LocationConfig>::const_iterator it = server._locations.begin(); it != server._locations.end(); ++it)
    {
        std::string rootPath = normalizePath(it->getRoot().empty() ? server._root : it->getRoot());
        std::string locationPath = normalizePath(it->getPath());
        if (!locationPath.empty() && locationPath != "/")
        {
            if (ServerUtils::isDirectory(server._root + locationPath) && 
                (rootPath.length() < locationPath.length() ||
                 rootPath.substr(rootPath.length() - locationPath.length()) != locationPath))
            {
                rootPath += locationPath;
            }
        }

        if (matchLocationPath(url, locationPath) && normalizedPath.compare(0, rootPath.length(), rootPath) == 0 &&
            (normalizedPath.length() == rootPath.length() || normalizedPath[rootPath.length()] == '/'))
        {
            if (rootPath.length() > bestMatchLength)
            {
                bestMatch = &(*it);
                bestMatchLength = rootPath.length();
            }
        }
    }
    return bestMatch;
}

std::string ResolvePaths::buildFilePathFromLocation(const Server &server, const LocationConfig *bestMatch, const std::string &locationIndex)
{
    std::string filePath;

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
            if (!std::ifstream(testPath.c_str()).good())
                root += locationPath;
        }
        filePath = root;
    }
    return filePath;
}


bool ResolvePaths::findLocationConfig(Server &server, const std::string &path, std::string &locationIndex, bool &autoindexEnabled, std::string &filePath, const std::string &url)
{
    std::string normalizedPath = normalizePath(path);
    if (normalizedPath.find(server._root) != 0)
        normalizedPath = server._root + normalizedPath;
    const LocationConfig *bestMatch = findBestLocationMatchForConfig(server, normalizedPath, url);
    if (bestMatch)
    {
        locationIndex = bestMatch->getIndex();
        autoindexEnabled = bestMatch->isAutoindexEnabled();
        filePath = buildFilePathFromLocation(server, bestMatch, locationIndex);
        return true;
    }
    return false;
}

std::string ResolvePaths::buildFullLocationPath(const std::string &normalizedPath, const std::string &root, const std::string &locationPath)
{
    std::string fullLocationPath = root;
    std::string fileName;

    size_t lastSlash = normalizedPath.find_last_of("/");
    if (lastSlash != std::string::npos)
        fileName = normalizedPath.substr(lastSlash + 1);
    else 
        fileName = normalizedPath;

    if (fileName.find(".") == std::string::npos)
        fileName = "";

    std::string testPath = root + "/" + fileName;
    bool isDir = ServerUtils::isDirectory(testPath);

    if (!isDir && !std::ifstream(testPath.c_str()).good())
        fullLocationPath = root + locationPath;
    else if (isDir && ServerUtils::isDirectory(root + locationPath))
        fullLocationPath = root + locationPath;
    return fullLocationPath;
}

bool ResolvePaths::findLocation(Server &server, const std::string &path)
{
    std::string normalizedPath = normalizePath(path);
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
            fullLocationPath = buildFullLocationPath(normalizedPath, normalizePath(it->getRoot()), locationPath);
        fullLocationPath = normalizePath(fullLocationPath);
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

bool ResolvePaths::matchLocationPath(const std::string &url, const std::string &locationPath)
{
    std::string normalizedUrl = doubleNormalizePath(url);
    std::string normalizedLocation = doubleNormalizePath(locationPath);
    if (normalizedLocation == "/" && (normalizedUrl == "/" || normalizedUrl.find("/") == 0))
        return true;
    return (normalizedUrl == normalizedLocation || normalizedUrl.find(normalizedLocation + "/") == 0);
}


bool ResolvePaths::isMethodAllowed(Server &server, const std::string &path, const std::string &method, const std::string &url)
{
    std::string normalizedPath = normalizePath(path);
    const LocationConfig *bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (size_t i = 0; i < server._locations.size(); i++)
    {
        std::string locationPath = normalizePath(server._locations[i].getPath());
        std::string fullLocationPath;
        if (server._locations[i].getAlias().empty() && server._locations[i].getRoot().empty())
            fullLocationPath = normalizePath(server._root + locationPath);
        else if (!server._locations[i].getAlias().empty()) 
            fullLocationPath = normalizePath(server._locations[i].getAlias());
        else
            fullLocationPath = buildFullLocationPath(normalizedPath, normalizePath(server._locations[i].getRoot()), locationPath);
        fullLocationPath = normalizePath(fullLocationPath);
        if (normalizedPath.find(fullLocationPath) == 0 && matchLocationPath(url, locationPath))
        {
            if (fullLocationPath.length() > bestMatchLength)
            {
                bestMatch = &server._locations[i];
                bestMatchLength = fullLocationPath.length();
            }
        }
    }
    if (bestMatch)
    {
        const std::vector<std::string> &methods = bestMatch->getMethods();
        // if (methods.empty()) 
        //     return (method == "GET");
        return std::find(methods.begin(), methods.end(), method) != methods.end();
    }

    return false; 
}

std::string ResolvePaths::urlDecode(const std::string& str)
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

std::map<std::string, std::string> ResolvePaths::parseURLEncoded(const std::string& data)
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

std::string ResolvePaths::extractPathFromURL(const std::string &url)
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

std::string ResolvePaths::normalizePath(const std::string &path)
{
    std::string normalized = path;

    size_t pos = normalized.find('?');
    if (pos != std::string::npos)
        normalized = normalized.substr(0, pos);
    while (normalized.length() > 1 && normalized[normalized.length() - 1] == '/')
        normalized.erase(normalized.length() - 1);
    return normalized;
}

std::string ResolvePaths::doubleNormalizePath(const std::string &path)
{
    if (path.empty())
        return "/";
    std::string normalized;
    size_t start = 0;
    while (start < path.length() && path[start] == '/')
        ++start;
    size_t end = path.length();
    while (end > start && path[end - 1] == '/')
        --end;
    if (start < end)
        normalized = path.substr(start, end - start);
    else
        normalized = "";
    size_t pos = normalized.find('?');
    if (pos != std::string::npos)
        normalized = normalized.substr(0, pos);
    return normalized.empty() ? "/" : "/" + normalized;
}

std::vector<char> ResolvePaths::decodeChunked(const std::string &requestStr)
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
