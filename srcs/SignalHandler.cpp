#include "SignalHandler.hpp"

volatile sig_atomic_t stopRequested = 0;

void handleSignal(int sig)
{
    (void)sig;
    const char *msg = "Signal received: stopping the server.\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    stopRequested = 1;
}