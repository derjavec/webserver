#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP


//basics
# include <cstring>
# include <iostream>
# include <unistd.h>
# include <cstdlib>
# include <cerrno>
# include <exception>
# include <sstream>
# include <fstream>
# include <map>
# include <vector>
# include <dirent.h>


//cgi
# include <sys/types.h>
# include <sys/wait.h>

//connections
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/epoll.h>
# include <fcntl.h>

//owns
//#include "Client.hpp"
//#include "Server.hpp"

#endif