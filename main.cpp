#include "Webserver.hpp"
#include "Client.hpp"
#include "Server.hpp"


int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1)
        return (std::cerr << "Usage: No arguments expected.", 1);
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