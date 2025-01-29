#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "Webserver.hpp"
#include "Config.hpp"

enum HttpMethod { GET, POST, DELETE, INVALID };

class HttpRequestParser
{
    private:
        HttpMethod _method;
        std::string _path;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::string _body;
        std::string _fileType;

    public:
        HttpRequestParser();
        void parseRequest(const std::string& rawRequest);

        HttpMethod getMethod() const;
        std::string getPath() const;
        std::string getVersion() const;
        std::map<std::string, std::string> getHeaders() const;
        std::string getBody() const;
        std::string getType() const;

    void parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    void validateRequest() const;
    std::string getContentType(const std::string& filePath);
    int stringToInt(const std::string& str);

    HttpMethod stringToMethod(const std::string& methodStr) const;
};

#endif
