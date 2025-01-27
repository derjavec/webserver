#include "LocationConfig.hpp"

LocationConfig::LocationConfig() : _autoindex(false), _clientMaxBodySize(1048576) {}
LocationConfig::~LocationConfig() {}
LocationConfig::LocationConfig(const LocationConfig& obj)
{
    *this = obj;
}
LocationConfig& LocationConfig::operator=(const LocationConfig& obj)
{
    if (this != &obj)
    {
        _path = obj._path;
        _root = obj._root;
        _index = obj._index;
        _autoindex = obj._autoindex;
        _redirect = obj._redirect;
        _cgiPath = obj._cgiPath;
        _cgiExt = obj._cgiExt;
        _clientMaxBodySize = obj._clientMaxBodySize;
        _methods = obj._methods;
    }
    return *this;
}

void LocationConfig::setPath(const std::string& path) { _path = path; }
void LocationConfig::setRoot(const std::string& root) { _root = root; }
void LocationConfig::setIndex(const std::string& index) { _index = index; }
void LocationConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void LocationConfig::addCgiPath(const std::string& path) { _cgiPath.push_back(path); }
void LocationConfig::addCgiExtension(const std::string& ext) { _cgiExt.push_back(ext); }
void LocationConfig::setRedirect(const std::string& redirect) { _redirect = redirect; }
void LocationConfig::setClientMaxBodySize(unsigned long size) { _clientMaxBodySize = size; }
void LocationConfig::addMethod(const std::string& method) { _methods.push_back(method); }

const std::string& LocationConfig::getPath() const { return _path; }
const std::string& LocationConfig::getRoot() const { return _root; }
const std::string& LocationConfig::getIndex() const { return _index; }
bool LocationConfig::isAutoindexEnabled() const { return _autoindex; }
const std::vector<std::string>& LocationConfig::getCgiPath() const { return _cgiPath; }
const std::vector<std::string>& LocationConfig::getCgiExt() const { return _cgiExt; }
const std::string& LocationConfig::getRedirect() const { return _redirect; }
unsigned long LocationConfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
const std::vector<std::string>& LocationConfig::getMethods() const { return _methods; }

void LocationConfig::parse(std::ifstream& configFile)
{
    std::string line;
    while (std::getline(configFile, line))
    {
        line = line.substr(0, line.find('#'));
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty())
            continue;

        size_t delimiterPos = line.find(" ");
        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        setParameter(key, value);

        if (key == "path") _path = value;
        else if (key == "root") _root = value;
        else if (key == "index") _index = value;
        else if (key == "autoindex") _autoindex = (value == "on");
        else if (key == "redirect") _redirect = value;
        else if (key == "cgi_path") _cgiPath.push_back(value);
        else if (key == "cgi_ext") _cgiExt.push_back(value);
        else if (key == "client_max_body_size") {
            std::stringstream ss(value);
            ss >> _clientMaxBodySize;
            if (ss.fail()) {
                throw std::runtime_error("Invalid client_max_body_size value: " + value);
            }
        }
        else if (key == "method") _methods.push_back(value);
    }
}

void LocationConfig::validate() const
{
    if (parameters.find("path") == parameters.end() || parameters.at("path").empty())
        throw std::runtime_error("LocationConfig: Missing or empty 'path' parameter.");
    if (parameters.find("root") == parameters.end() || parameters.at("root").empty())
        throw std::runtime_error("LocationConfig: Missing or empty 'root' parameter.");
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
            unsigned long bodySize = std::stoul(parameters.at("client_max_body_size"));
            if (bodySize <= 0)
                throw std::runtime_error("LocationConfig: 'client_max_body_size' must be a positive number.");
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("LocationConfig: Invalid value for 'client_max_body_size'. Must be a positive number.");
        }
    }

    if (parameters.find("methods") != parameters.end())
    {
        const std::string& methods = parameters.at("methods");
        std::istringstream methodStream(methods);
        std::string method;
        while (methodStream >> method)
        {
            if (method != "GET" && method != "POST" && method != "DELETE" &&
                method != "PUT" && method != "HEAD" && method != "OPTIONS")
                throw std::runtime_error("LocationConfig: Invalid method '" + method + "' in 'methods'.");
        }
    }
    if (parameters.find("cgi_path") != parameters.end() && parameters.find("cgi_ext") == parameters.end())
        throw std::runtime_error("LocationConfig: 'cgi_path' defined but 'cgi_ext' is missing.");
    if (parameters.find("cgi_ext") != parameters.end() && parameters.find("cgi_path") == parameters.end())
        throw std::runtime_error("LocationConfig: 'cgi_ext' defined but 'cgi_path' is missing.");
    if (parameters.find("redirect") != parameters.end() && parameters.at("redirect").empty())
        throw std::runtime_error("LocationConfig: 'redirect' is defined but empty.");

    std::cout << "LocationConfig validated successfully: " << parameters.at("path") << std::endl;
}

