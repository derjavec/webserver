#include "HttpRequestParser.hpp"

HttpRequestParser::HttpRequestParser(): _method(INVALID), _path("/"), _version("HTTP/1.1"), _body("") {}

void HttpRequestParser::parseRequest(const std::string& rawRequest) 
{
    std::istringstream stream(rawRequest);
    std::string line;

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

std::string HttpRequestParser::getContentType(const std::string& filePath)
{
    if (!filePath.empty() && filePath[filePath.size() - 1] == '/')
        return "text/html";
    size_t dotPos = filePath.find_last_of(".");
    if (dotPos == std::string::npos)
        return "application/octet-stream";
    static std::map<std::string, std::string> mimeTypes;
    if (mimeTypes.empty())
    {
        mimeTypes.insert(std::make_pair("html", "text/html"));
        mimeTypes.insert(std::make_pair("css", "text/css"));
        mimeTypes.insert(std::make_pair("js", "application/javascript"));
        mimeTypes.insert(std::make_pair("jpg", "image/jpeg"));
        mimeTypes.insert(std::make_pair("jpeg", "image/jpeg"));
        mimeTypes.insert(std::make_pair("png", "image/png"));
        mimeTypes.insert(std::make_pair("gif", "image/gif"));
        mimeTypes.insert(std::make_pair("ico", "image/x-icon"));
        mimeTypes.insert(std::make_pair("json", "application/json"));
        mimeTypes.insert(std::make_pair("xml", "application/xml"));
        mimeTypes.insert(std::make_pair("pdf", "application/pdf"));
        mimeTypes.insert(std::make_pair("txt", "text/plain"));
        mimeTypes.insert(std::make_pair("mp4", "video/mp4"));
        mimeTypes.insert(std::make_pair("webm", "video/webm"));
        mimeTypes.insert(std::make_pair("ogg", "video/ogg"));
        mimeTypes.insert(std::make_pair("mp3", "audio/mpeg"));
        mimeTypes.insert(std::make_pair("wav", "audio/wav"));
    }
    std::string extension = filePath.substr(dotPos + 1);
    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(extension);
    if (it != mimeTypes.end())
        return it->second;
    return "application/octet-stream";
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
std::string HttpRequestParser::getType() const { return _fileType; }
std::string HttpRequestParser::getVersion() const { return _version; }
std::map<std::string, std::string> HttpRequestParser::getHeaders() const { return _headers; }
std::string HttpRequestParser::getBody() const { return _body; }
