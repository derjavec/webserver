#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "Webserver.hpp"
#include "Config.hpp"

enum HttpMethod { GET, POST, DELETE, PUT, INVALID };

class HttpRequestParser
{
    private:
        HttpMethod _method;
        std::string _path;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::string _body;
        std::pair<std::string, std::string> _fileType;

    public:
        HttpRequestParser();
        void parseRequest(const std::string& rawRequest);

        HttpMethod getMethod() const;
        void setPath(const std::string& path);
        std::string getPath() const;
        std::string getVersion() const;
        std::map<std::string, std::string> getHeaders() const;
        std::string getBody() const;
        void setType(const std::string& path);
        std::pair<std::string, std::string> getType() const;

    void parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    void validateRequest() const;
    std::pair<std::string, std::string> getContentType(const std::string& filePath);
    int stringToInt(const std::string& str);
    std::string ToString(HttpMethod method);

    HttpMethod stringToMethod(const std::string& methodStr) const;
};

#endif
