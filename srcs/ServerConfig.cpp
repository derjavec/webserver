#include "ServerConfig.hpp"

ServerConfig::ServerConfig(): _ports(), _serverName("localhost"), _root("./"), _clientMaxBodySize(1048576), _index("index.html"), _autoindex(false) , _upload("www/upload") {}
ServerConfig::~ServerConfig(){}
ServerConfig::ServerConfig(const ServerConfig& obj): Config(obj)
{
    *this = obj;
}
ServerConfig& ServerConfig::operator=(const ServerConfig& obj)
{
    if (this != &obj)
    {
        _ports = obj._ports;
        _serverName = obj._serverName;
        _root = obj._root;
        _upload = obj._upload;
        _clientMaxBodySize = obj._clientMaxBodySize;
        _index = obj._index;
        _autoindex = obj._autoindex;
        _errorPages = obj._errorPages;
        _locations = obj._locations;
    }
    return *this;
}

const std::vector<uint16_t> ServerConfig::getPort() const { return _ports; }
const std::string& ServerConfig::getServerName() const { return _serverName; }
const std::string& ServerConfig::getRoot() const { return _root; }
const std::string& ServerConfig::getUpload() const { return _upload; }
unsigned long ServerConfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
const std::string& ServerConfig::getIndex() const { return _index; }
bool ServerConfig::isAutoindexEnabled() const { return _autoindex; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _errorPages; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return _locations; }

void ServerConfig::setPort(std::vector<uint16_t> ports) { _ports = ports; }
void ServerConfig::setServerName(const std::string& name) { _serverName = name; }
void ServerConfig::setRoot(const std::string& root) { _root = root; }
void ServerConfig::setUpload(const std::string& upload) { _upload = upload; }
void ServerConfig::setClientMaxBodySize(unsigned long size) { _clientMaxBodySize = size; }
void ServerConfig::setIndex(const std::string& index) { _index = index; }
void ServerConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void ServerConfig::addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
void ServerConfig::addLocation(const LocationConfig& location) {_locations.push_back(location);}

void ServerConfig::validateSingleValue(const std::string& key, const std::string& value)
{
    if (key == "error_page")
        return;
    
    if (key != "listen") 
    {
        size_t spacePos = value.find(" ");
        if (spacePos != std::string::npos)
        {
            std::string extra = value.substr(spacePos + 1);
            extra.erase(0, extra.find_first_not_of(" \t"));
            extra.erase(extra.find_last_not_of(" \t") + 1);
            if (!extra.empty() && extra != ";")
                throw std::runtime_error("ServerConfig: Parameter '" + key + "' contains invalid extra values: " + extra);
        }
    }
}

// const std::vector<uint16_t> ServerConfig::getPort() const
// {
// 	return (this->_ports);
// }

void ServerConfig::parseServerKeyValue(const std::string& key, const std::string& value)
{
    validateSingleValue(key, value);
    if (key == "listen")
    {
    	std::istringstream iss(value);
        std::string token;
        while (iss >> token)
        {
            int port = stringToInt(token);
            if (port < 0 || port > 65635)
                throw std::runtime_error("ServerConfig: Invalid port number:" + token);
            _ports.push_back(static_cast<uint16_t>(port));
        }

    }
    else if (key == "server_name")
        _serverName = value;
    else if (key == "upload_store")
    _upload = value;
    else if (key == "root")
        _root = value;
    else if (key == "client_max_body_size")
        _clientMaxBodySize = stringToULong(value);
    else if (key == "index")
        _index = value;
    else if (key == "autoindex")
    {
        if (value == "on")
            _autoindex = true;
        else if (value == "off")
            _autoindex = false;
        else
            throw std::runtime_error("Invalid value for 'autoindex'. Expected 'on' or 'off'.");
    }
    else if (key == "error_page")
    {
        size_t spacePos = value.find(" ");
        if (spacePos == std::string::npos)
            throw std::runtime_error("ServerConfig: 'error_page' must have a code and a path."); 
        std::string errorCodeStr = value.substr(0, spacePos);
        std::string errorPagePath = value.substr(spacePos + 1);
        int errorCode = stringToInt(errorCodeStr);
        if (errorPagePath.empty())
            throw std::runtime_error("ServerConfig: 'error_page' must include a path.");
        _errorPages[errorCode] = errorPagePath;
        setParameter("server_error_page_" + numberToString(errorCode), errorPagePath);
    }
    else
        throw std::runtime_error("ServerConfig: Unknown directive '" + key + "' in server block.");
}



void ServerConfig::parse(std::ifstream& configFile)
{
    std::string line;
    LocationConfig currentLocation;
    bool inLocationBlock = false;
    int braceDepth = 0;

    while (std::getline(configFile, line))
    {
        line = line.substr(0, line.find('#'));
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        if (line.empty())
            continue;
        if ((line.find("server") != std::string::npos && line.find("{") != std::string::npos) 
        || (line.find("server{") != std::string::npos))
        {
            braceDepth++;
            continue;
        }
        
        if (line.find("location ") != std::string::npos && line.find("{") != std::string::npos)
        {
            size_t pathStart = line.find("location ") + 9;
            size_t pathEnd = line.find("{", pathStart);
            std::string path = line.substr(pathStart, pathEnd - pathStart);
            path.erase(0, path.find_first_not_of(" \t"));
            path.erase(path.find_last_not_of(" \t;") + 1);
            currentLocation = LocationConfig();
            currentLocation.setParameter("path", path);
            currentLocation.setPath(path);
            inLocationBlock = true;
            ++braceDepth;
            continue;
        }

        if (line == "}")
        {
            --braceDepth;
            if (inLocationBlock)
            {
                currentLocation.validate();
                _locations.push_back(currentLocation);
                inLocationBlock = false;
                //currentLocation.print();
            }
            else if (braceDepth != 0)
                throw std::runtime_error("ServerConfig: Unmatched closing brace '}' in configuration.");
            continue;
        }

        size_t delimiterPos = line.find_first_of(" \t");
        std::string key = line.substr(0, delimiterPos);

        std::string value;
        if (delimiterPos != std::string::npos)
            value = line.substr(delimiterPos + 1);
        else
            value = "";
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t;") + 1);
        if (inLocationBlock)
        {
            currentLocation.setParameter(key, value);
            if (key == "root") currentLocation.setRoot(value);
            else if (key == "alias") currentLocation.setAlias(value);
            else if (key == "index") currentLocation.setIndex(value);
            else if (key == "autoindex")
            {
                if (value == "on")
                    currentLocation.setAutoindex(true);
                else if (value == "off")
                    currentLocation.setAutoindex(false);
                else
                    throw std::runtime_error("Invalid value for 'autoindex'. Expected 'on' or 'off'.");
            }
            else if (key == "redirect") currentLocation.setRedirect(value);
            else if (key == "cgi_path") currentLocation.addCgiPath(value);
            else if (key == "cgi_ext") currentLocation.addCgiExtension(value);
            else if (key == "client_max_body_size") currentLocation.setClientMaxBodySize(stringToULong(value));
            else if (key == "allow_methods")
	    {
		    std::istringstream methodStream(value);
		    std::string method;
		    while (methodStream >> method)
		    {
			    if (!method.empty() && method[method.length() - 1] == ';')
				    method = method.substr(0, method.length() - 1);
			    currentLocation.addMethod(method);
		    }
	    } 
	    else if (key == "method")
		    throw std::runtime_error("Invalid key 'method'. Did you mean 'allow_methods'?");
	} 
        else
        {
            setParameter("server_" + key, value);
            parseServerKeyValue(key, value);
        }
    }
}


void ServerConfig::validate() const
{
    if (parameters.find("server_listen") == parameters.end())
        throw std::runtime_error("ServerConfig: Missing 'listen' parameter.");
    if (parameters.find("server_root") == parameters.end()) 
    {
        for (std::vector<LocationConfig>::const_iterator it = _locations.begin(); it != _locations.end(); ++it) 
        {
            if (it->getRedirect().empty() && it->getRoot().empty() && it->getAlias().empty()) 
                throw std::runtime_error("ServerConfig: Missing 'root' parameter. At least one location has no root or alias.");
        }
    }          
    if (parameters.find("server_index") == parameters.end())
        throw std::runtime_error("ServerConfig: Missing 'index' parameter.");
    if (!directoryExists(_upload))
        throw std::runtime_error("ServerConfig: Upload directory does not exist: " + _upload);
    if (!directoryExists(_root))
        throw std::runtime_error("ServerConfig: Root directory does not exist: " + _root);
}

void ServerConfig::print() const
{
    std::cout << "=== ServerConfig Parsed ===" << std::endl;
   // std::cout << "Port: " << _port << std::endl;
    std::cout << "Server Name: " << _serverName << std::endl;
    std::cout << "Root: " << _root << std::endl;
    std::cout << "Client Max Body Size: " << _clientMaxBodySize << std::endl;
    std::cout << "Index: " << _index << std::endl;
    std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;

    std::cout << "Error Pages: " << std::endl;
    for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it)
    {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }

    std::cout << "Locations: " << std::endl;
    for (size_t i = 0; i < _locations.size(); ++i)
    {
        std::cout << "  Location " << i + 1 << ":" << std::endl;
        _locations[i].print();
    }

    std::cout << "Parameters Map:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = parameters.begin(); it != parameters.end(); ++it)
    {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "=========================" << std::endl;

}
