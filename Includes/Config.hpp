#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "Webserver.hpp"

class Config
{
    protected :
        std::map<std::string, std::string> parameters;

    public :
        Config();
        virtual ~Config();
        Config(const Config& obj);
        Config& operator=(const Config& obj);
        
        virtual void validate() const = 0;
        void setParameter(const std::string& key, const std::string& value);
        const std::string& getParameter(const std::string& key) const;
        bool validateConfigFile(const std::string& filePath, std::ifstream& configFile);
        unsigned long stringToULong(const std::string& str) const;
        static bool stringToBool(const std::string& str);
        std::string numberToString(int number);
};
int stringToInt(const std::string& str);
#endif