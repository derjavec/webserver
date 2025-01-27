#include "Config.hpp"

Config::Config() {}
Config::~Config() {}
Config::Config(const Config& obj)
{
    *this = obj;
}
Config& Config::operator=(const Config& obj)
{
    if (this != &obj)
    {
        parameters = obj.parameters;
    }
    return *this;
}
void Config::setParameter(const std::string& key, const std::string& value)
{
    parameters[key] = value;
}
const std::string& Config::getParameter(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = parameters.find(key);
    if (it == parameters.end())
    {
        throw std::runtime_error("Parameter not found: " + key);
    }
    return it->second;
}

bool Config::validateConfigFile(const std::string& filename) {
    if (filename.substr(filename.find_last_of(".") + 1) != "conf")
    {
        std::cerr << "Error: Config file must have a .conf extension." << std::endl;
        return false;
    }

    std::ifstream configFile(filename.c_str());
    if (!configFile.is_open())
    {
        std::cerr << "Error: Unable to open config file." << std::endl;
        return false;
    }

    if (configFile.peek() == std::ifstream::traits_type::eof())
    {
        std::cerr << "Error: Config file is empty." << std::endl;
        return false;
    }

    std::string line;
    bool hasListen = false, hasServerName = false, hasRoot = false;
    while (std::getline(configFile, line))
    {
        line = line.substr(0, line.find('#'));
        if (line.find("listen") != std::string::npos) hasListen = true;
        if (line.find("server_name") != std::string::npos) hasServerName = true;
        if (line.find("root") != std::string::npos) hasRoot = true;
    }
    configFile.close();

    if (!hasListen || !hasServerName || !hasRoot)
    {
        std::cerr << "Error: Config file must contain 'listen', 'server_name', and 'root' directives." << std::endl;
        return false;
    }

    return true;
}

int Config::stringToInt(const std::string& str)
{
    std::stringstream ss(str);
    int value;
    ss >> value;
    if (ss.fail())
        throw std::runtime_error("Invalid integer: " + str);
    return value;
}

unsigned long Config::stringToULong(const std::string& str)
{
    std::stringstream ss(str);
    unsigned long value;
    ss >> value;
    if (ss.fail())
        throw std::runtime_error("Invalid unsigned long: " + str);
    return value;
}

bool Config::stringToBool(const std::string& str)
{
    if (str == "true" || str == "on" || str == "1")
        return true;
    if (str == "false" || str == "off" || str == "0")
        return false;
    throw std::runtime_error("Invalid boolean: " + str);
}