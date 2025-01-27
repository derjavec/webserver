#include "Webserver.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Config.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"


int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file.conf>" << std::endl;
        return 1;
    }
    std::string configFile = argv[1];
    ServerConfig serverConfig;
    if (!serverConfig.validateConfigFile(configFile))
        return 1;
    try
    {

        Server server(8080);
        server.run();
    }
    catch (const ServerException &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }   
    catch (const std::exception &e)
    {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
    }     
    return 0;
}