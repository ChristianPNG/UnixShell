/*
	361 Unix Shell Project
	by: Christian Rullan
*/


#include "sh.h"
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
void sig_handler(int signal);

int main( int argc, char **argv, char **envp )
{
  /* put signal set up stuff here */
  struct sigaction signal = {0};
  signal.sa_handler = &sig_handler;
  signal.sa_flags = SA_RESTART;
  signal.sa_flags = 0;
  sigaction(SIGINT, &signal, NULL);
  sigaction(SIGTSTP, &signal, NULL);
  sigaction(SIGTERM, &signal, NULL);
  return sh(argc, argv, envp);
}

void sig_handler(int signal)
{
  write(1, "\n", 1);
  return;
}
