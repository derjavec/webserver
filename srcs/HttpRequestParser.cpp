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

HttpMethod HttpRequestParser::getMethod() const { return _method; }
std::string HttpRequestParser::getPath() const { return _path; }
std::string HttpRequestParser::getVersion() const { return _version; }
std::map<std::string, std::string> HttpRequestParser::getHeaders() const { return _headers; }
std::string HttpRequestParser::getBody() const { return _body; }
