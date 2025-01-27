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
        virtual void parse(std::ifstream& configFile) = 0;
        virtual void validate() const = 0;
        void setParameter(const std::string& key, const std::string& value);
        const std::string& getParameter(const std::string& key) const;

        bool validateConfigFile(const std::string& filename);

        int stringToInt(const std::string& str);
        unsigned long stringToULong(const std::string& str);
        static bool stringToBool(const std::string& str);
};
#endif