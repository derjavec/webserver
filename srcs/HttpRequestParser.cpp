#include "HttpRequestParser.hpp"

HttpRequestParser::HttpRequestParser(): _method(INVALID), _path("/"), _version("HTTP/1.1"), _body("") {}

void HttpRequestParser::parseRequest(const std::string& rawRequest) 
{
    std::istringstream stream(rawRequest);
    std::string line;
    std::cout << "Raw request: " << rawRequest << std::endl;
    if (std::getline(stream, line))
        parseRequestLine(line);
    else
        throw std::runtime_error("Invalid request: Missing request line.");
    while (std::getline(stream, line) && line != "\r")
    {
        parseHeader(line);
    }
    if (_headers.count("Content-Length") > 0)
    {
        size_t contentLength = stringToInt(_headers["Content-Length"]);
        _body.resize(contentLength);
        stream.read(&_body[0], contentLength);
    }
    validateRequest();
}

HttpMethod HttpRequestParser::stringToMethod(const std::string& methodStr) const 
{
    if (methodStr == "GET")
        return GET;
    if (methodStr == "POST")
        return POST;
    if (methodStr == "PUT")
        return PUT;
    if (methodStr == "DELETE")
        return DELETE;
    return INVALID;
}

void HttpRequestParser::parseRequestLine(const std::string& line)
{
    std::istringstream lineStream(line);
    std::string methodStr, path, version;

    if (!(lineStream >> methodStr >> path >> version)) 
        throw std::runtime_error("Invalid request: Malformed request line.");
    _method = stringToMethod(methodStr);
    if (_method == INVALID)
        throw std::runtime_error("Invalid request: Unsupported HTTP method.");
    _path = path;
    _version = version;
    if (_version != "HTTP/1.1")
        throw std::runtime_error("Invalid request: Unsupported HTTP version.");
    _fileType = getContentType(_path);
    std::cout << "fileType: " << _fileType.first << " " << _fileType.second << std::endl;
}

void HttpRequestParser::parseHeader(const std::string& line)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
        throw std::runtime_error("Invalid request: Malformed header.");
    std::string name = line.substr(0, colonPos);
    std::string value = line.substr(colonPos + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t\r") + 1);
    _headers[name] = value;
}

void HttpRequestParser::validateRequest() const
{
    if (_method == INVALID)
        throw std::runtime_error("Invalid request: Unsupported HTTP method.");
    if (_version != "HTTP/1.1")
        throw std::runtime_error("Invalid request: Unsupported HTTP version.");
}

int HttpRequestParser::stringToInt(const std::string& str)
{
    std::stringstream ss(str);
    int value;
    ss >> value;
    if (ss.fail())
        throw std::runtime_error("Invalid integer: " + str);
    return value;
}

std::pair<std::string, std::string> HttpRequestParser::getContentType(const std::string& filePath)
{
    std::cout << filePath << std::endl;
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
    std::cout << "🔍 Checking filePath: " << cleanPath << std::endl;
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

std::string HttpRequestParser::ToString(HttpMethod method)
{
    switch (method)
    {
        case GET: return "GET";
        case POST: return "POST";
        case PUT: return "PUT";
        case DELETE: return "DELETE";
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
