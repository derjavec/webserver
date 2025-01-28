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

bool Config::validateConfigFile(const std::string& filePath, std::ifstream& configFile)
{
    size_t dotPos = filePath.rfind('.');
    if (dotPos == std::string::npos || (filePath.substr(dotPos) != ".conf" && filePath.substr(dotPos) != ".config"))
    {
        std::cerr << "Error: Configuration file must have a .conf or .config extension." << std::endl;
        return false;
    }
    if (!configFile.is_open())
        throw std::runtime_error("Error: Configuration file could not be opened.");
    if (configFile.peek() == std::ifstream::traits_type::eof())
        throw std::runtime_error("Error: Configuration file is empty.");
    std::string line;
    int openBraces = 0;
    while (std::getline(configFile, line))
    {
        for (size_t i = 0; i < line.size(); ++i)
        {
            if (line[i] == '{') 
                ++openBraces;
            else if (line[i] == '}')
                --openBraces;

            if (openBraces < 0)
            {
                std::cerr << "Error: Unbalanced braces in configuration file." << std::endl;
                return false;
            }
        }
    }
    if (openBraces != 0)
    {
        std::cerr << "Error: Unbalanced braces in configuration file." << std::endl;
        return false;
    }
    configFile.clear();
    configFile.seekg(0);
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

unsigned long Config::stringToULong(const std::string& str) const
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

std::string Config::numberToString(int number)
{
    std::stringstream ss;
    ss << number;
    return ss.str();
}
