#include "LocationConfig.hpp"

LocationConfig::LocationConfig() :  _root(""), _alias(""), _autoindex(false), _clientMaxBodySize(1048576){}
LocationConfig::~LocationConfig() {}
LocationConfig::LocationConfig(const LocationConfig& obj) : Config(obj)
{
    *this = obj;
}
LocationConfig& LocationConfig::operator=(const LocationConfig& obj)
{
    if (this != &obj)
    {
        _path = obj._path;
        _root = obj._root;
        _alias = obj._alias;
        _index = obj._index;
        _autoindex = obj._autoindex;
        _redirect = obj._redirect;
        _cgiPath = obj._cgiPath;
        _cgiExt = obj._cgiExt;
        _clientMaxBodySize = obj._clientMaxBodySize;
        _methods = obj._methods;
        parameters = obj.parameters;
    }
    return *this;
}

void LocationConfig::setPath(const std::string& path) { _path = path; }
void LocationConfig::setRoot(const std::string& root) { _root = root; }
void LocationConfig::setAlias(const std::string& alias) { _alias = alias; }
void LocationConfig::setIndex(const std::string& index) { _index = index; }
void LocationConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void LocationConfig::addCgiPath(const std::string& path) { _cgiPath.push_back(path); }
void LocationConfig::addCgiExtension(const std::string& ext) { _cgiExt.push_back(ext); }
void LocationConfig::setRedirect(const std::string& redirect) { _redirect = redirect; }
void LocationConfig::setClientMaxBodySize(unsigned long size) { _clientMaxBodySize = size; }
void LocationConfig::addMethod(const std::string& method) { _methods.push_back(method); }

const std::string& LocationConfig::getPath() const { return _path; }
const std::string& LocationConfig::getRoot() const { return _root; }
const std::string& LocationConfig::getAlias() const { return _alias; }
const std::string& LocationConfig::getIndex() const { return _index; }
bool LocationConfig::isAutoindexEnabled() const { return _autoindex; }
const std::vector<std::string>& LocationConfig::getCgiPath() const { return _cgiPath; }
const std::vector<std::string>& LocationConfig::getCgiExt() const { return _cgiExt; }
const std::string& LocationConfig::getRedirect() const { return _redirect; }
unsigned long LocationConfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
const std::vector<std::string>& LocationConfig::getMethods() const { return _methods; }


void LocationConfig::validate() const
{
    if (parameters.find("path") == parameters.end())
        throw std::runtime_error("LocationConfig: Missing 'path' parameter.");
    if (parameters.at("path").empty() && parameters.at("path") != "/")
        throw std::runtime_error("LocationConfig: 'path' parameter is empty or invalid.");
    if (parameters.find("redirect") == parameters.end())
    {
        bool has_root = (parameters.find("root") != parameters.end() && !parameters.at("root").empty());
        bool has_alias = (parameters.find("alias") != parameters.end() && !parameters.at("alias").empty());
        if (has_root && has_alias)
            throw std::runtime_error("LocationConfig: 'root' and 'alias' cannot be used together.");
    }
    if (parameters.find("autoindex") != parameters.end())
    {
        const std::string& autoindex = parameters.at("autoindex");
        if (autoindex != "on" && autoindex != "off")
            throw std::runtime_error("LocationConfig: Invalid value for 'autoindex'. Must be 'on' or 'off'.");
    }
    if (parameters.find("client_max_body_size") != parameters.end())
    {
        try
        {
            unsigned long bodySize = stringToULong(parameters.at("client_max_body_size"));
            if (bodySize <= 0)
                throw std::runtime_error("LocationConfig: 'client_max_body_size' must be a positive number.");
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("LocationConfig: Invalid value for 'client_max_body_size'. Must be a positive number.");
        }
    }
    if (parameters.find("allow_methods") != parameters.end())
    {
        const std::string& methods = parameters.at("allow_methods");
        std::istringstream methodStream(methods);
        std::string method;
        while (methodStream >> method)
        {
            if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE" && method != "HEAD" && method != "OPTIONS" && method != "PATCH" && method != "TRACE" && method != "CONNECT" )
                throw std::runtime_error("LocationConfig: Invalid method '" + method + "' in 'methods'.");
        }
    }
    if (parameters.find("cgi_path") != parameters.end() && parameters.find("cgi_ext") == parameters.end())
        throw std::runtime_error("LocationConfig: 'cgi_path' defined but 'cgi_ext' is missing.");
    if (parameters.find("cgi_ext") != parameters.end() && parameters.find("cgi_path") == parameters.end())
        throw std::runtime_error("LocationConfig: 'cgi_ext' defined but 'cgi_path' is missing.");
    if (parameters.find("redirect") != parameters.end() && parameters.at("redirect").empty())
        throw std::runtime_error("LocationConfig: 'redirect' is defined but empty.");
}


void LocationConfig::print() const
{
    std::cout << "=== LocationConfig Parsed ===" << std::endl;
    std::cout << "    Path: " << _path << std::endl;
    std::cout << "    Root: " << _root << std::endl;
    std::cout << "    Index: " << _index << std::endl;
    std::cout << "    Autoindex: " << (_autoindex ? "on" : "off") << std::endl;

    std::cout << "    CGI Paths: ";
    for (size_t i = 0; i < _cgiPath.size(); ++i) {
        std::cout << _cgiPath[i] << (i == _cgiPath.size() - 1 ? "" : ", ");
    }
    std::cout << std::endl;

    std::cout << "    CGI Extensions: ";
    for (size_t i = 0; i < _cgiExt.size(); ++i) {
        std::cout << _cgiExt[i] << (i == _cgiExt.size() - 1 ? "" : ", ");
    }
    std::cout << std::endl;

    std::cout << "    Redirect: " << (_redirect.empty() ? "None" : _redirect) << std::endl;
    std::cout << "    Client Max Body Size: " << _clientMaxBodySize << std::endl;

    std::cout << "    Methods: ";
    for (size_t i = 0; i < _methods.size(); ++i)
    {
        std::cout << _methods[i] << (i == _methods.size() - 1 ? "" : ", ");
    }
    std::cout << std::endl;

    std::cout << "    Parameters Map:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = parameters.begin(); it != parameters.end(); ++it) {
        std::cout << "      " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "=========================" << std::endl;
}


