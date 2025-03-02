#include "HttpRequestParser.hpp"

HttpRequestParser::HttpRequestParser(): _method(INVALID), _path("/"), _version("HTTP/1.1"), _body("") {}

void HttpRequestParser::parseCookies(const std::string &cookieHeader)
{
    std::istringstream stream(cookieHeader);
    std::string cookiePair;
    while (std::getline(stream, cookiePair, ';'))
    {
        size_t equalPos = cookiePair.find('=');
        if (equalPos != std::string::npos)
        {
            std::string key = cookiePair.substr(0, equalPos);
            std::string value = cookiePair.substr(equalPos + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            value.erase(0, value.find_first_not_of(" \t"));
            _cookies[key] = value;
        }
    }
}
std::map<std::string, std::string> HttpRequestParser::getCookies(void)
{
    return (this->_cookies);
}

int HttpRequestParser::parseRequest(const std::vector<char>& rawRequest)
{
    int res;
    std::istringstream stream(std::string(rawRequest.begin(), rawRequest.end()));
    std::string line;
    if (std::getline(stream, line))
    {
        res = parseRequestLine(line);
        if (res != 0)
            return res;
    }
    else
    {
        std::cerr << "Error: 400 Invalid request: Missing request line." << std::endl;
        return 400;
    }
    while (std::getline(stream, line) && line != "\r")
    {
        res = parseHeader(line);
        if (res != 0)
            return res;
        if (line.find("Cookie:") == 0)
        {
            std::string cookieHeader = line.substr(7);
            parseCookies(cookieHeader);
        }
    }
    if (_headers.count("Content-Length") > 0)
    {
        int contentLength = 0;
        if (!stringToInt(_headers["Content-Length"], contentLength))
            return 400;
        _body.resize(contentLength);
        if (contentLength > 0)
            stream.read(&_body[0], contentLength);
        else if (contentLength < 0)
             return 400;
    }
    res = validateRequest();
    if (res != 0)
        return res;
    return 0;
}

HttpMethod HttpRequestParser::stringToMethod(const std::string& methodStr) const 
{
    if (methodStr == "GET")
        return GET;
    if (methodStr == "POST")
        return POST;
    if (methodStr == "DELETE")
        return DELETE;
    if (methodStr == "PUT")
        return PUT;
    if (methodStr == "HEAD")
        return HEAD;
    if (methodStr == "PATCH")
        return PATCH;
    if (methodStr == "OPTIONS")
        return OPTIONS;
    if (methodStr == "TRACE")
        return TRACE;
    if (methodStr == "CONNECT")
        return CONNECT;
    return INVALID;
}

bool HttpRequestParser::hasExtraSpaces(const std::string& line)
{
    bool foundSpace = false;

    for (size_t i = 0; i < line.length(); i++)
    {
        if (line[i] == ' ')
        {
            if (foundSpace)
                return true;
            foundSpace = true;
        }
        else
        {
            foundSpace = false;
        }
    }
    return false;
}

int HttpRequestParser::parseRequestLine(const std::string& line)
{
    std::istringstream lineStream(line);
    std::string methodStr, path, version, extra;

    if (hasExtraSpaces(line))
    {
        std::cerr << "Error: 400 Bad Request: Malformed request line." << std::endl;
        return 400;
    } 
    if (!(lineStream >> methodStr >> path >> version) || (lineStream >> extra))
    {
        std::cerr << "Error: 400 Bad Request: Malformed request line." << std::endl;
        return 400;
    } 
    if (path.length() > MAX_URI_LENGTH)
    {
        std::cerr << "Error: 414 Request-URI Too Long." << std::endl;
        return 414;
    } 
    _method = stringToMethod(methodStr);
    if (path.empty() || path[0] != '/')
    {
        std::cerr << "Error: 400 Bad Request: Invalid request path." << std::endl;
        return 400;
    }
    _path = path;
    _version = version;
    _fileType = getContentType(_path);
    return (0);
}


int HttpRequestParser::parseHeader(const std::string& line)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
    {
        std::cerr << "Error: 400 Invalid request: Missing request line." << std::endl;
        return 400;
    }
    std::string name = line.substr(0, colonPos);
    std::string value = line.substr(colonPos + 1);
    if (name.empty() || name.find(' ') != std::string::npos || name.find('\t') != std::string::npos)
    {
        std::cerr << "Error: 400 Bad Request: Invalid header format." << std::endl;
        return 400;
    } 
    if (name.empty())
    {
        std::cerr << "Error: 400 Bad Request: Empty header name." << std::endl;
        return 400;
    } 
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t\r") + 1);
    if (value.empty() && (name == "Host" || name == "Content-Length" || name == "User-Agent"))
    {
        std::cerr << "Error: 400 Bad Request: Empty header value." << std::endl;
        return 400;
    } 
    for (int i = 0; i < 4; i++)
    {
        if (name == nonRepeatableHeaders[i] && _headers.find(name) != _headers.end())
        {
            std::cerr << "Error: 400 Bad Request: Duplicate " << name << " header." << std::endl;
            return 400;
        }
    }
    if (_headers.find(name) != _headers.end())
        _headers[name] += ", " + value;
    else
        _headers[name] = value;

    return 0;
    return (0);
}


int HttpRequestParser::validateRequest() const
{
    if (_method == INVALID)
    {
        std::cerr << "❌ Invalid request: Unsupported HTTP method." << std::endl;
        return 501;
    }
    if (_method == PUT || _method == HEAD || _method == OPTIONS || _method == PATCH || _method == TRACE || _method == CONNECT)
    {
        std::cerr << "❌ Invalid request: Not implemented HTTP method." << std::endl;
        return 405;
    }
    if (_version != "HTTP/1.1")
    {
        std::cerr << "❌ Invalid request: Unsupported HTTP version." << std::endl;
        return 400;
    }
    if (_method == POST)
    {
        if (_headers.count("Content-Length") == 0 && _headers.count("Transfer-Encoding") == 0 && !_body.empty())
        {
            std::cerr << "❌ Error: 400 Bad Request: POST request without Content-Length or Transfer-Encoding." << std::endl;
            return 400;
        }
    }
    return 0;
}


bool HttpRequestParser::stringToInt(const std::string& str, int &value)
{
    std::stringstream ss(str);
    ss >> value;
    if (ss.fail())
    {
        std::cerr << "Invalid integer: " << str <<std::endl;
        return false;
    }      
    return true;
}

std::pair<std::string, std::string> HttpRequestParser::getContentType(const std::string& filePath)
{
    if (!filePath.empty() && filePath[filePath.size() - 1] == '/')
        return std::make_pair("text/html", "html");
    size_t dotPos = filePath.find_last_of(".");
    if (dotPos == std::string::npos)
        return std::make_pair("application/octet-stream", "unknown");
    size_t pos = filePath.find("?");
    std::string cleanPath; 
    if (pos != std::string::npos)
        cleanPath = filePath.substr(0, pos);
    else
        cleanPath = filePath;
    static std::map<std::string, std::pair<std::string, std::string > > mimeTypes;
    if (mimeTypes.empty())
    {
        // docs and text
        mimeTypes["html"] = std::make_pair("text/html", "html");
        mimeTypes["htm"] = std::make_pair("text/html", "html");
        mimeTypes["css"] = std::make_pair("text/css", "static");
        mimeTypes["js"] = std::make_pair("application/javascript", "static");
        mimeTypes["json"] = std::make_pair("application/json", "static");
        mimeTypes["xml"] = std::make_pair("application/xml", "static");
        mimeTypes["txt"] = std::make_pair("text/plain", "static");
        mimeTypes["csv"] = std::make_pair("text/csv", "static");
        mimeTypes["pdf"] = std::make_pair("application/pdf", "static");
        mimeTypes["yaml"] = std::make_pair("text/yaml", "static");
        mimeTypes["md"] = std::make_pair("text/markdown", "static");

        // images
        mimeTypes["png"] = std::make_pair("image/png", "image");
        mimeTypes["jpg"] = std::make_pair("image/jpeg", "image");
        mimeTypes["jpeg"] = std::make_pair("image/jpeg", "image");
        mimeTypes["gif"] = std::make_pair("image/gif", "image");
        mimeTypes["svg"] = std::make_pair("image/svg+xml", "image");
        mimeTypes["ico"] = std::make_pair("image/x-icon", "image");
        mimeTypes["bmp"] = std::make_pair("image/bmp", "image");
        mimeTypes["tiff"] = std::make_pair("image/tiff", "image");
        mimeTypes["webp"] = std::make_pair("image/webp", "image");

        // videos
        mimeTypes["mp4"] = std::make_pair("video/mp4", "video");
        mimeTypes["webm"] = std::make_pair("video/webm", "video");
        mimeTypes["ogg"] = std::make_pair("video/ogg", "video");
        mimeTypes["avi"] = std::make_pair("video/x-msvideo", "video");
        mimeTypes["mov"] = std::make_pair("video/quicktime", "video");
        mimeTypes["mkv"] = std::make_pair("video/x-matroska", "video");

        // audio
        mimeTypes["mp3"] = std::make_pair("audio/mpeg", "audio");
        mimeTypes["wav"] = std::make_pair("audio/wav", "audio");
        mimeTypes["ogg"] = std::make_pair("audio/ogg", "audio");
        mimeTypes["flac"] = std::make_pair("audio/flac", "audio");
        mimeTypes["aac"] = std::make_pair("audio/aac", "audio");
        mimeTypes["m4a"] = std::make_pair("audio/mp4", "audio");

        // binary files
        mimeTypes["exe"] = std::make_pair("application/x-msdownload", "binary");
        mimeTypes["dll"] = std::make_pair("application/x-msdownload", "binary");
        mimeTypes["bin"] = std::make_pair("application/octet-stream", "binary");

        // compressed files
        mimeTypes["zip"] = std::make_pair("application/zip", "compressed");
        mimeTypes["tar"] = std::make_pair("application/x-tar", "compressed");
        mimeTypes["gz"] = std::make_pair("application/gzip", "compressed");
        mimeTypes["bz2"] = std::make_pair("application/x-bzip2", "compressed");
        mimeTypes["7z"] = std::make_pair("application/x-7z-compressed", "compressed");
        mimeTypes["rar"] = std::make_pair("application/vnd.rar", "compressed");

        // scripts
        mimeTypes["php"] = std::make_pair("application/x-httpd-php", "script");
        mimeTypes["py"] = std::make_pair("text/x-python", "script");
        mimeTypes["cpp"] = std::make_pair("text/x-c++src", "script");
        mimeTypes["h"] = std::make_pair("text/x-c", "script");
        mimeTypes["java"] = std::make_pair("text/x-java-source", "script");
        mimeTypes["sh"] = std::make_pair("application/x-sh", "script");
        mimeTypes["pl"] = std::make_pair("text/x-perl", "script");
        mimeTypes["rb"] = std::make_pair("text/x-ruby", "script");
    }
    std::string extension = cleanPath.substr(dotPos + 1);
    std::map<std::string, std::pair<std::string, std::string > >::const_iterator it = mimeTypes.find(extension);
    if (it != mimeTypes.end())
        return it->second;
    return std::make_pair("application/octet-stream", "unknown2");
}

const std::string HttpRequestParser::ToString(const HttpMethod &method)
{
    switch (method)
    {
        case GET: return "GET";
        case POST: return "POST";
        case DELETE: return "DELETE";
        case PUT: return "PUT";
        case PATCH: return "PATCH";
        case OPTIONS: return "OPTIONS";
        case HEAD: return "HEAD";
        case TRACE: return "TRACE";
        case CONNECT: return "CONNECT";
        default: return "UNKNOWN";
    }
}

HttpMethod HttpRequestParser::getMethod() const { return _method; }
std::string HttpRequestParser::getPath() const { return _path; }
void HttpRequestParser::setPath(const std::string& path) { _path = path; }
std::pair<std::string, std::string> HttpRequestParser::getType() const { return _fileType; }
void HttpRequestParser::setType(const std::string& path) { _fileType = getContentType(path); }
std::string HttpRequestParser::getVersion() const { return _version; }
std::map<std::string, std::string> HttpRequestParser::getHeaders() const { return _headers; }
std::string HttpRequestParser::getBody() const { return _body; }
