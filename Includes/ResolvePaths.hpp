#ifndef RESOLVEPATHS_HPP
#define RESOLVEPATHS_HPP

# include "Server.hpp"
# include "Webserver.hpp"
# include "HttpRequestParser.hpp"
# include "LocationConfig.hpp"
// # include <sys/stat.h>

//class Server;

class ResolvePaths
{      
    public:
        static const LocationConfig* findBestLocation(Server &server, std::string &path, size_t &bestMatchLength);
        static std::string computeResolvedPath(const Server &server, const LocationConfig *bestLocation, const std::string &reqPath, std::string &relativePath);
        static std::string resolveFilePath(Server &server, HttpRequestParser parser);
        static std::map<std::string, std::string> parseURLEncoded(const std::string& data);
        static std::string urlDecode(const std::string& str);      
        static std::vector<char> decodeChunked(const std::string &requestStr);
        static std::string doubleNormalizePath(const std::string &path);
        static std::string normalizePath(const std::string &path);  
        static bool findLocation(Server &server, std::string &url);
        //static const LocationConfig* findBestLocationMatchForConfig(Server &server, const std::string &normalizedPath, const std::string &url);
        static std::string buildFilePathFromLocation(const Server &server, const LocationConfig *bestMatch, const std::string &locationIndex);
        static std::string buildFullLocationPath(const std::string &normalizedPath, const std::string &root, const std::string &locationPath);
        static bool findLocationConfig(Server &server, std::string &locationIndex, bool &autoindexEnabled, std::string &url);
        //static bool matchLocationPath(const std::string &url, const std::string &locationPath);
        static bool isMethodAllowed(Server &server, const std::string &path, const std::string &method, std::string &url);
        static std::string extractPathFromURL(const std::string &url);      
};

#endif