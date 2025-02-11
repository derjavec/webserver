#include "Webserver.hpp"
#include "Server.hpp"
#include "Config.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"


int main(int argc, char **argv)
{
     if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file.conf>" << std::endl;
        return 1;
    } else if (argc == 1){
    	argv[1] = (char*)"Config/default.conf";
    }

    std::string configFilePath = argv[1];
    std::ifstream configFile(configFilePath.c_str());
    try
    {
        ServerConfig serverConfig;
        if (!serverConfig.validateConfigFile(configFilePath, configFile))
            return 1; 
        serverConfig.parse(configFile);
        serverConfig.validate();
        std::cout << "Server configuration successfully parsed:\n";
       // serverConfig.print();

        Server server(serverConfig);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
