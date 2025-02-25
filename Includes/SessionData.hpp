
#ifndef SESSIONDATA_HPP
#define SESSIONDATA_HPP

#include <string>
#include <ctime>


struct SessionData 
{
    std::string sessionId;
    bool isLoggedIn;
    std::time_t connectionTime;
};

#endif
