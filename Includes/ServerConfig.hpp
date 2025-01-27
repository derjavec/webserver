#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "Webserver.hpp"
#include "Config.hpp"
#include "LocationConfig.hpp"

class ServerConfig : public Config
{   
    private:
        uint16_t _port;
        std::string _serverName;
        std::string _root;
        size_t _clientMaxBodySize;
        std::string _index;
        bool _autoindex;
        std::map<int, std::string> _errorPages;
        std::vector<LocationConfig> _locations;

    public :
        ServerConfig();
        virtual ~ServerConfig();
        ServerConfig(const ServerConfig& obj);
        ServerConfig& operator=(const ServerConfig& obj);

        void parse(std::ifstream& configFile);
        void validate() const;

        void setPort(uint16_t port);
        uint16_t getPort() const;

        void setServerName(const std::string& name);
        const std::string& getServerName() const;

        void setRoot(const std::string& root);
        const std::string& getRoot() const;

        void setClientMaxBodySize(unsigned long size);
        unsigned long getClientMaxBodySize() const;

        void setIndex(const std::string& index);
        const std::string& getIndex() const;

        void setAutoindex(bool autoindex);
        bool isAutoindexEnabled() const;

        void addErrorPage(int code, const std::string& path);
        const std::map<int, std::string>& getErrorPages() const;

        void addLocation(const LocationConfig& location);
        const std::vector<LocationConfig>& getLocations() const;

        void print() const;
};
#endif