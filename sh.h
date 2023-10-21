
#include "get_path.h"
extern int pid;
extern char **environ;
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
void where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void execute(char *commandpath, char**args, char**envp, int status);
void printenv(char **envp);
char** wildcard(char**args, int i, int length, char* commandline);
void freeAll(char* prompt, char* commandline, char** args, char* pwd, char* owd, char *pathHolder, struct pathelement *pathlist);
void killPID(char **args, char* prompt, char* commandline, char* pwd, char* owd, char *pathHolder, struct pathelement *pathlist);
void printenv(char **args);
struct pathelement *setenviron(char **args, struct pathelement *pathlist, char *homedir);
void catcher();
#define PROMPTMAX 32
#define MAXARGS 10
