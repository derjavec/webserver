#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "Webserver.hpp"
#include "Config.hpp"

enum HttpMethod { GET, POST, DELETE, PUT, HEAD, PATCH, OPTIONS, TRACE, CONNECT, INVALID};
const std::string nonRepeatableHeaders[4] = {
    "Content-Length", "Host", "Transfer-Encoding", "Content-Type"};

class HttpRequestParser
{
    private:
        HttpMethod _method;
        std::string _path;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::string _body;
        std::pair<std::string, std::string> _fileType;
        std::map<std::string, std::string> _cookies;

    public:
        HttpRequestParser();
        void parseCookies(const std::string &cookieHeader);
        std::map<std::string, std::string> getCookies(void);
        int parseRequest(const std::vector<char>& rawRequest);

        HttpMethod getMethod() const;
        void setPath(const std::string& path);
        std::string getPath() const;
        std::string getVersion() const;
        std::map<std::string, std::string> getHeaders() const;
        std::string getBody() const;
        void setType(const std::string& path);
        std::pair<std::string, std::string> getType() const;
        bool hasExtraSpaces(const std::string& line);
        int parseRequestLine(const std::string& line);
        int parseHeader(const std::string& line);
        int validateRequest() const;
        std::pair<std::string, std::string> getContentType(const std::string& filePath);
        bool stringToInt(const std::string& str, int &value);
        const std::string ToString(const HttpMethod &method);

    HttpMethod stringToMethod(const std::string& methodStr) const;
};

#endif
