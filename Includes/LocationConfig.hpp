#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include "Webserver.hpp"
#include "Config.hpp"
#include "ServerUtils.hpp"

class LocationConfig : public Config
{
    private:
        std::string _path;
        std::string _root;
        std::string _alias;
        std::string _index;
        bool _autoindex;
        std::vector<std::string> _cgiPath;
        std::vector<std::string> _cgiExt;
        std::string _redirect;
        unsigned long _clientMaxBodySize;
        std::vector<std::string> _methods;

    public:
        LocationConfig();
        ~LocationConfig();
        LocationConfig(const LocationConfig& obj);
        LocationConfig& operator=(const LocationConfig& obj);

        void validate() const;
        
        void setPath(const std::string& path);
        const std::string& getPath() const;

        void setRoot(const std::string& root);
        const std::string& getRoot() const;

        void setAlias(const std::string& alias);
        const std::string& getAlias() const;

        void setIndex(const std::string& index);
        const std::string& getIndex() const;

        void setAutoindex(bool autoindex);
        bool isAutoindexEnabled() const;

        void addCgiPath(const std::string& path);
        const std::vector<std::string>& getCgiPath() const;

        void addCgiExtension(const std::string& ext);
        const std::vector<std::string>& getCgiExt() const;

        void setRedirect(const std::string& redirect);
        const std::string& getRedirect() const;

        void setClientMaxBodySize(unsigned long size);
        unsigned long getClientMaxBodySize() const;

        void addMethod(const std::string& method);
        const std::vector<std::string>& getMethods() const;

        void print() const;
};

#endif
