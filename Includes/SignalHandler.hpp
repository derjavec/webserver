#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include "Webserver.hpp"

extern volatile sig_atomic_t stopRequested;
void handleSignal(int sig);

#endif