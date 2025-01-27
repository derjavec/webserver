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
        if (line.find("location ") != std::string::npos && line.find("{") != std::string::npos)
        {
            currentLocation = LocationConfig();
            inLocationBlock = true;
            continue;
        }

        if (line == "}")
        {
           if (inLocationBlock)
            {
                _locations.push_back(currentLocation);
                inLocationBlock = false;
            }
            continue;
        }

        if (inLocationBlock)
        {
            size_t delimiterPos = line.find(" ");
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            currentLocation.setParameter(key, value);

            if (key == "path") currentLocation.setPath(value);
            else if (key == "root") currentLocation.setRoot(value);
            else if (key == "index") currentLocation.setIndex(value);
            else if (key == "autoindex") currentLocation.setAutoindex(value == "on");
            else if (key == "redirect") currentLocation.setRedirect(value);
            else if (key == "cgi_path") currentLocation.addCgiPath(value);
            else if (key == "cgi_ext") currentLocation.addCgiExtension(value);
            else if (key == "client_max_body_size") currentLocation.setClientMaxBodySize(stringToULong(value));
            else if (key == "method") currentLocation.addMethod(value);
        }
        else
        {
            size_t delimiterPos = line.find(" ");
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            setParameter("server_" + key, value);

            if (key == "listen")
                _port = stringToInt(value);
            else if (key == "server_name") _serverName = value;
            else if (key == "root") _root = value;
            else if (key == "client_max_body_size") _clientMaxBodySize = stringToULong(value);
            else if (key == "index") _index = value;
            else if (key == "autoindex") _autoindex = (value == "on");
            else if (key == "error_page")
            {
                size_t spacePos = value.find(" ");
                int errorCode = stringToInt(value.substr(0, spacePos));
                std::string errorPage = value.substr(spacePos + 1);
                _errorPages[errorCode] = errorPage;
                setParameter("server_error_page_" + numberToString(errorCode), errorPage);
            }
        }
    }
}

void ServerConfig::validate() const
{
    if (parameters.find("server_listen") == parameters.end())
        throw std::runtime_error("ServerConfig: Missing 'listen' parameter.");
    if (parameters.find("server_root") == parameters.end())
        throw std::runtime_error("ServerConfig: Missing 'root' parameter.");
    if (parameters.find("server_index") == parameters.end())
        throw std::runtime_error("ServerConfig: Missing 'index' parameter.");
}

void ServerConfig::print() const
{
    std::cout << "=== ServerConfig Parsed ===" << std::endl;
    std::cout << "Port: " << _port << std::endl;
    std::cout << "Server Name: " << _serverName << std::endl;
    std::cout << "Root: " << _root << std::endl;
    std::cout << "Client Max Body Size: " << _clientMaxBodySize << std::endl;
    std::cout << "Index: " << _index << std::endl;
    std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;

    std::cout << "Error Pages: " << std::endl;
    for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }

    std::cout << "Locations: " << std::endl;
    for (size_t i = 0; i < _locations.size(); ++i) {
        std::cout << "  Location " << i + 1 << ":" << std::endl;
        _locations[i].print();
    }

    std::cout << "Parameters Map:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = parameters.begin(); it != parameters.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "=========================" << std::endl;

}