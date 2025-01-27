#include "ServerConfig.hpp"

ServerConfig::ServerConfig(): _port(8080), _serverName("localhost"), _root("./"), _clientMaxBodySize(1048576), _index("index.html"), _autoindex(false) {}
ServerConfig::~ServerConfig(){}
ServerConfig::ServerConfig(const ServerConfig& obj)
{
    *this = obj;
}
ServerConfig& ServerConfig::operator=(const ServerConfig& obj)
{
    if (this != &obj)
    {
        _port = obj._port;
        _serverName = obj._serverName;
        _root = obj._root;
        _clientMaxBodySize = obj._clientMaxBodySize;
        _index = obj._index;
        _autoindex = obj._autoindex;
        _errorPages = obj._errorPages;
        _locations = obj._locations;
    }
    return *this;
}

uint16_t ServerConfig::getPort() const { return _port; }
const std::string& ServerConfig::getServerName() const { return _serverName; }
const std::string& ServerConfig::getRoot() const { return _root; }
unsigned long ServerConfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
const std::string& ServerConfig::getIndex() const { return _index; }
bool ServerConfig::isAutoindexEnabled() const { return _autoindex; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _errorPages; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return _locations; }

void ServerConfig::setPort(uint16_t port) { _port = port; }
void ServerConfig::setServerName(const std::string& name) { _serverName = name; }
void ServerConfig::setRoot(const std::string& root) { _root = root; }
void ServerConfig::setClientMaxBodySize(unsigned long size) { _clientMaxBodySize = size; }
void ServerConfig::setIndex(const std::string& index) { _index = index; }
void ServerConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void ServerConfig::addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
void ServerConfig::addLocation(const LocationConfig& location) {_locations.push_back(location);}





void ServerConfig::parse(std::ifstream& configFile)
{
    std::string line;
    LocationConfig currentLocation;
    bool inLocationBlock = false;

    while (std::getline(configFile, line))
    {
        line = line.substr(0, line.find('#')); 
        line.erase(0, line.find_first_not_of(" \t")); 
        line.erase(line.find_last_not_of(" \t") + 1); 

        if (line.empty())
            continue;

        if (line == "location {")
        {
            currentLocation = LocationConfig();
            inLocationBlock = true;
            continue;
        }
        if (line == "}")
        {
            if (inLocationBlock)
            {
                currentLocation.parse(line);
                _locations.push_back(currentLocation);
                inLocationBlock = false;
            }
            continue;
        }

        size_t delimiterPos = line.find(" ");
        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        setParameter("server_" + key, value);

        if (key == "listen") _port = stringToInt(value);
        else if (key == "server_name") _serverName = value;
        else if (key == "root") _root = value;
        else if (key == "client_max_body_size") _clientMaxBodySize = stringToULong(value);
        else if (key == "index") _index = value;
        else if (key == "autoindex") _autoindex = (value == "on");
        else if (key == "error_page")
        {
            size_t spacePos = value.find(" ");
            int errorCode = std::stoi(value.substr(0, spacePos));
            std::string errorPage = value.substr(spacePos + 1);
            _errorPages[errorCode] = errorPage;
            setParameter("server_error_page_" + std::to_string(errorCode), errorPage);
        }
    }
}

void ServerConfig::validate() const
{
    if (parameters.find("listen") == parameters.end())
        throw std::runtime_error("ServerConfig: Missing 'listen' parameter.");
    if (parameters.find("root") == parameters.end())
        throw std::runtime_error("ServerConfig: Missing 'root' parameter.");
    if (parameters.find("index") == parameters.end())
        throw std::runtime_error("ServerConfig: Missing 'index' parameter.");
}
