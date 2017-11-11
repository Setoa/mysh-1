#include "signal_handlers.h"
#include <signal.h>
#include <unistd.h>
void catch_sigint(int signalNo)
{
  // TODO: File this!
	signal(SIGINT,SIG_IGN);
	sleep(1);
}

void catch_sigtstp(int signalNo)
{
  // TODO: File this!
	signal(SIGTSTP,SIG_IGN);
	sleep(1);
}
