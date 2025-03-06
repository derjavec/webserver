#include "ResolvePaths.hpp"

const LocationConfig* ResolvePaths::findBestLocation(Server &server, std::string path, size_t &bestMatchLength)
{
    const LocationConfig* bestMatch = NULL;
    path = doubleNormalizePath(normalizePath(path));
    if (!path.empty() && path[path.size() - 1] != '/')
            path = path + "/";
    for (size_t i = 0; i < server._locations.size(); ++i)
    {
        std::string locPath = normalizePath(server._locations[i].getPath());
        if (!locPath.empty() && locPath[locPath.size() - 1] != '/')
            locPath = locPath + "/";
        if (path.find(locPath) == 0)
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

std::string ResolvePaths::resolveFilePath(Server &server, HttpRequestParser parser)
{
    size_t bestMatchLength = 0;
    std::string url = ResolvePaths::doubleNormalizePath(parser.getPath());
    const LocationConfig* bestLocation = findBestLocation(server, url, bestMatchLength);
    std::string relativePath;
    if (bestMatchLength >= url.size())
        relativePath = "";
    else
        relativePath= url.substr(bestMatchLength);
    std::string resolvedPath;
   
    if (bestLocation)
    {
        if (bestLocation->getAlias().empty() && bestLocation->getRoot().empty())
        resolvedPath = server._root + url;
        else if (!bestLocation->getAlias().empty())
        {
            std::string alias = normalizePath(bestLocation->getAlias());
            resolvedPath = alias + "/" + relativePath;
        }
        else
        {
            std::string root = normalizePath(bestLocation->getRoot());
            resolvedPath = root + "/" + relativePath;
        }
    } 
    else
    {
        std::string root = normalizePath(server._root);   
        if (url.find('.') != std::string::npos && url.find(".html") != std::string::npos)
            resolvedPath = root + "/html/" + url;
        else
            resolvedPath = root + url;
    }
    return urlDecode(resolvedPath);
}

bool ResolvePaths::findLocationConfig(Server &server, std::string &locationIndex, bool &autoindexEnabled,  std::string &url)
{
    size_t bestMatchLength = 0;
    const LocationConfig *bestMatch = findBestLocation(server, url, bestMatchLength);
    if (bestMatch)
    {
        
        locationIndex = bestMatch->getIndex();
        autoindexEnabled = bestMatch->isAutoindexEnabled();
        return true;
    }
    return false;
}

bool ResolvePaths::isLocation(Server &server, std::string &url)
{
    size_t bestMatchLength = 0;
    const LocationConfig *bestMatch = findBestLocation(server, url, bestMatchLength);
    return bestMatch != NULL;
}

bool ResolvePaths::isMethodAllowed(Server &server, const std::string &path, const std::string &method, std::string &url)
{
    std::string normalizedPath = normalizePath(path);
    size_t bestMatchLength = 0;
    const LocationConfig *bestMatch = findBestLocation(server, url, bestMatchLength);

    if (bestMatch)
    {
        const std::vector<std::string> &methods = bestMatch->getMethods();
        return std::find(methods.begin(), methods.end(), method) != methods.end();
    }
    return false; 
}

std::vector<std::string> ResolvePaths::getAllowedMethods(Server &server, const std::string &url)
{
    std::string normalizedUrl = normalizePath(url);
    size_t bestMatchLength = 0;
    const LocationConfig *bestMatch = findBestLocation(server, normalizedUrl, bestMatchLength);

    if (bestMatch)
        return bestMatch->getMethods();  
    std::vector<std::string> defaultMethods;
    defaultMethods.push_back("GET");
    return defaultMethods;
}

std::string urlDecode(const std::string& str)
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
    if (path.empty() || path == "/")
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
    return (normalized.empty() || normalized[0] == '.' || normalized[0] == '/') ? normalized : "/" + normalized;
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