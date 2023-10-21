#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sh.h"
#include <sys/stat.h>
#include <libgen.h>
#include <glob.h>

int sh( int argc, char **argv, char **envp )
/*
 Params: takes in the initial command (argc) and initial arguments (argv) and initial environement variables
 	when the program was started
 Description: Runs a mimic Unix Shell with built in commands + external commands. Can only be successfully exited
	with no memory leaks using the exit command.
 returns: An int that will go back to the main function indicating a successful or unsuccessful exit.
*/
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *pwd, *owd, *dir;
  char **args = calloc(253, sizeof(char*));
  int uid, status = 0, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  int counter = 0;

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  char* pathHolder = (char*)calloc(strlen(pwd)+1, sizeof(char));
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();
  strcpy(pathHolder, pwd);
  while ( go )
  {
	/*
	  First early parts of the while loop handles any updates done to the homedir, current
	  working directory, old working directory, and prints the command prompt once its all
	  taken care of.
	*/
    free(pwd);
	if (strcmp(homedir, getenv("HOME")) != 0){
		homedir = getenv("HOME");
	}
	if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL ){
    	perror("getcwd");
    	exit(2);
    }
	if (strcmp(owd, pwd) != 0){
		if (counter > 0){
			free(owd);
			owd = calloc(strlen(pathHolder)+1, sizeof(char));
			strcpy(owd, pathHolder);
		}
		free(pathHolder);
		pathHolder = (char*)calloc(strlen(pwd)+1, sizeof(char));
		strcpy(pathHolder, pwd);
		counter++;
	}
	else{
		if (counter > 0){
			free(owd);
			owd = calloc(strlen(pathHolder)+1, sizeof(char));
			strcpy(owd, pathHolder);
			free(pathHolder);
			pathHolder = (char*)calloc(strlen(pwd)+1, sizeof(char));
			strcpy(pathHolder, pwd);
		}
	}
    	printf("%s[%s]>", prompt, pwd);
	fflush(stdout);
    	if (fgets(commandline, MAX_CANON, stdin) == NULL){
		clearerr(stdin);
		printf("\n");
		continue;
	}
	commandline[strcspn(commandline, "\n")] = 0; //makes \n into 0 at the end

	/*
	  Once an input is retrieved from the command prompt it will be split up and stored into
	  an args array which is a char**. It ends with NULL and the length is stored in old_i.
	*/
    char *token = strtok(commandline," ");
	command = token;
	int i = 1;
	args[0] = command;
	token = strtok(NULL, " ");
	while(token){
		args[i] = token;
		i++;
		token = strtok(NULL, " ");
	}
	args[i] = NULL;
	int old_i = i;


	/*
	  Now its time to run through the list of built in commands to check if the input given
	  matches with any of them. If the command is empty just restart the prompt.
	*/
	if (command == NULL){
		continue;
	}
	else if (strcmp(command, "prompt") == 0){
	/*
	  if the command given is "prompt", the shell will then use the given prefix,
	  or ask for one, and place it in front of the command prompt after execution
	*/
		printf("Executing built-in: prompt\n");
		if (args[2] != NULL){
			printf("Error: Too many arguments\n");
			continue;
		}
		else if (args[1] == NULL){
			printf("	input prompt prefix: ");
			char x[PROMPTMAX];
			fgets(x, PROMPTMAX, stdin);
			x[strcspn(x, "\n")] = 0;
			args[1] = x;
			args[2] = NULL;
		}
		arg = args[1];
		strncpy(prompt, arg, PROMPTMAX);
		int len = (int)strlen(prompt);
		prompt[len] = '\0';
	}
	else if (strcmp(command, "which") == 0){
	  //If the given command is "which", loop through all the arguments and run the which function.
		printf("Executing built-in: which\n");
		i = 1;
		while(args[i] != NULL){
			arg = args[i];
			commandpath = which(arg, pathlist);
			printf("%s\n", commandpath);
			free(commandpath);
			i++;
		}
	}
	else if (strcmp(command, "pid") == 0){
		printf("Executing built-in: pid\n");
		pid_t currPid = getpid();
		printf("pid is: %d\n", currPid);
	}
	else if (strcmp(command, "exit") == 0){
		printf("Executing built-in: exit\n");
		freeAll(prompt, commandline, args, pwd, owd, pathHolder, pathlist);
		break;
	}
	else if (strcmp(command, "list") == 0){
	/*
	  Checks conditions before executing the list function. If no args, the pwd will be sent as an argument
	  to the list function. Else, all readable files will be sent to the list function one by one.
	*/
		printf("Executing built-in: list\n");
		if (args[1] == NULL){
			dir = pwd;
			list(dir);
		}
		else{
			i = 1;
			while (args[i] != NULL){
				if(access(args[i], R_OK) == 0){
					dir = args[i];
					list(dir);
				}
				i++;
			}
		}
	}
	else if (strcmp(command, "cd") == 0){
	/*
	  If the given command is "cd" it will change the directory to either the homedir if no args are given,
	  the old dir if '-' arg is given, or the given dir if the path is given by the user.
	*/
		printf("Executing built-in: cd\n");
		if (args[1] == NULL){
			chdir(homedir);
		}
		else if (args[2] != NULL){
			printf("Error: too many arguments\n");
		}
		else if (strcmp(args[1], "-") == 0){
			chdir(owd);
		}
		else{
			int cdStatus = chdir(args[1]);
			if (cdStatus != 0){
				counter = 0;
				perror("Error: ");
			}
		}
	}
	else if (strcmp(command, "where") == 0){
		printf("Executing built-in: where\n");
		int r = 1;
		while (args[r] != NULL){
			where(args[r], pathlist);
			r++;
		}
		if (args[1] == NULL){
			printf("Error: too few arguments\n");
		}
	}
	else if (strcmp(command, "kill") == 0){
		printf("Executing built-in: kill\n");
		killPID(args, prompt, commandline, pwd, owd, pathHolder, pathlist);
	}
	else if (strcmp(command, "printenv") == 0){
		printf("Executing built-in: printenv\n");
		printenv(args);
	}
	else if (strcmp(command, "setenv") == 0){
		printf("Executing built-in: setenv\n");
		pathlist = setenviron(args, pathlist, homedir);
	}
	else if (access(command,X_OK) == 0){
	//If the given command is a file on a given absolute path that exists, execute that file
		struct stat path_stat;
		stat(command, &path_stat);
		if (S_ISREG(path_stat.st_mode) != 0){
			execute(command, args, envp, status);
		}
		else{
			puts("Error: cannot execute a directory");
		}
	}
	else
    {
	/*
	  If none of the built in commands matched the given command. We will assume it is an external command
	  and try to execute it. First find out if it has a '*' or '?' wildcard. If it does run the wildcard function.
	  If the given input is unable to be executed an error will be printed out.
	*/
		for (i = 0; i < old_i; i++){
			if (strchr(args[i], '*') != NULL || strchr(args[i], '?') != NULL){
				args = wildcard(args, i, old_i, commandline);
			}
		}
		commandpath = which(command, pathlist);
		if (strcmp(commandpath, "error") != 0){
			execute(commandpath, args, envp, status);
		}
		else {
			fprintf(stderr, "%s: Command not found.\n", args[0]);
		}
		free(commandpath);
    }
  }
  return 0;
} /* sh() */

char *which(char *command, struct pathelement *pathlist)
{
	/*
	  Params: takes in the command we want to run which on. Also takes in pathlist which is
	  	a linked list of all the paths.
	  Description: Checks through all the paths in the pathlist until it encounters an executable matching
		the given command.
	  Returns: If it reaches a successful executable, the path will be stored in an allocated string and returned.
		If it doesn't it will return an allocated string containing the word "error".
	*/
	char *commandpath;
	char cmd[PATH_MAX];
	while (pathlist != NULL){
		sprintf(cmd, "%s/%s", pathlist->element,command);
		if (access(cmd,X_OK) == 0){
			cmd[strlen(cmd)] = '\0';
			commandpath = (char*)malloc(strlen(cmd)*sizeof(char)+1);
			strcpy(commandpath, cmd);
			return commandpath;
		}
		pathlist = pathlist->next;
	}
	char *tmp = "error";
	commandpath = (char*)malloc(6*sizeof(char));
	strcpy(commandpath, tmp);
	return commandpath;

}

void where(char *command, struct pathelement *pathlist )
{
  /*
	Params: Takes in the command string we want to run where on. It will also take in
	  pathlist which is a linked list to all paths in the enviornment.
	Description: This function will check for all the paths in the pathlist and print out every location
	  to where the command is located.
	Returns: void
  */
	char cmd[PATH_MAX];
	while (pathlist != NULL){
		sprintf(cmd, "%s/%s", pathlist->element,command);
		if (access(cmd,F_OK) == 0){
			cmd[strlen(cmd)] = '\0';
			printf("%s\n", cmd);
		}
		pathlist = pathlist->next;
	}
}

void list ( char *dir )
{
	/*
	  Params: A char* which is a string containing the path of the directory.
	  Description: Prints out all the files in a given directory line by line.
	  Returns: void
	*/
	struct stat path;
	stat(dir, &path);
	if (S_ISREG(path.st_mode) == 0){ //if file is dir
		DIR* openedDir = opendir(dir);
		struct dirent* entity;
		entity = readdir(openedDir);
		char *folderName = basename(dir);
		printf("\n%s:\n", folderName);
		while (entity != NULL){
			printf("%s\n", entity->d_name);
			entity = readdir(openedDir);
		}
		closedir(openedDir);
	}

}

void execute(char *commandpath, char **args, char **envp, int status){
	pid_t pid;
	if ((pid = fork()) < 0) {
		puts("ERROR");
	}
	else if (pid == 0) {
		printf("Executing: %s\n", commandpath);
		args[0] = commandpath;
		execve(commandpath, args, envp);
		puts("Error Failed Execution");
		exit(0);
	}
	else {
		signal(SIGINT, catcher);
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)){
			int exit_status = WEXITSTATUS(status);
			if (exit_status != 0){
				printf("Exit status of child was %d\n", exit_status);
			}
		}
	}
}

char** wildcard(char **args, int i, int length, char* commandline){
	/*
	  Params: List of arguments, char **args. int i which is the index of which the wildcard was found.
		The length of the arguments list. And the commandline which is the whole given input to the shell
		including the command and args in a single string format.

	  Description: This function will use the glob command and struct to execute the wildcards. It does this by
		getting filtering out all the files after reading the wildcard and appending all those files to the end of
		the given command. EX: "ls *" will be "ls file1 file 2 .." If there are no matches on the wildcard an error
		will be printed out and if a problem occurs in general another error will be printed out.

	  Returns: the new list of arguments in char **args. If the function encounters any problems args will be returned
		but the first string on the list will be "error".
	*/
	char **element;
	int returnStatus;
	glob_t globstruct;
	returnStatus = glob(args[i], GLOB_ERR, NULL, &globstruct);

	if (returnStatus != 0){
		if( returnStatus==GLOB_NOMATCH )
            fprintf(stderr, "No matches\n");
        else
            fprintf(stderr, "Error Executing\n ");
        args[0] = "error";
		return args;
	}
	else{
			element = globstruct.gl_pathv;

			char* commandlineTMP =(char*)calloc(MAX_CANON, sizeof(char));
			strcat(commandlineTMP, args[0]);
			strcat(commandlineTMP, " ");
			while (*element){
				strcat(commandlineTMP, *element);
				element++;
				strcat(commandlineTMP, " ");
			}
			for (int y = 1; y < length; y++){
				if ((strchr(args[y], '*') == NULL) && (strchr(args[y], '?') == NULL)){
					strcat(commandlineTMP, args[y]);
					strcat(commandlineTMP, " ");
				}
			}
			memset(commandline, 0, strlen(commandline));
			strcat(commandline, commandlineTMP);
			free(commandlineTMP);
			globfree(&globstruct);

			char *CLptr = commandline;
			char *tok;
			tok = strtok(CLptr," ");
			int k = 1;
			args[0] = tok;
			tok = strtok(NULL," ");
			while(tok != NULL){
				args[k] = tok;
				k++;
				tok = strtok(NULL," ");
			}
			args[k] = NULL;
			return args;
	}
}

void freeAll(char *prompt, char *commandline, char **args, char *pwd, char *owd, char *pathHolder, struct pathelement *pathlist){
	/*
	  Params: Takes in all variables that were dynamically allocated in this entire program including the pathlist.
	  Description: This function will free up all allocated memory and exit this Unix Shell Program.
	  Returns: void
	*/
	free(prompt);
	free(commandline);
	free(args);
	free(pwd);
	free(owd);
	free(pathHolder);
	struct pathelement *tmp = pathlist;
	free(pathlist->element);
	while (tmp != NULL){
		struct pathelement *tmp2 = tmp;
		tmp = tmp->next;
		free(tmp2);
	}
	free(tmp);
}
void killPID(char **args, char *prompt, char *commandline, char *pwd, char *owd, char *pathHolder, struct pathelement *pathlist){
	/*
	  Params: list of all allocated variables in the program. char **Args will be modified and the rest will only be called if sigterm
		or sigint is sent. Which case all these variables are freed before the signal executes.

	  Description: Given No argumnets, killPID will result in an error. Given a non int argument, killPID will also result
		in an error. Given one argument, killPID will find that Process ID (PID) and SIGTERM (terminate) it. Given two arguments
		the first argument must include a '-' or else an error is returned. The first of the two arguments will be the signal.
		(EX: -2, 2 will be the SIGINT signal). The second argument will be the PID. That pid will then be given the signal.

	  Returns: Void
	*/
	if (args[1] == NULL){
		printf("Error: kill requires arguments\n");
		return;
	}
	char *a = (char*)calloc(strlen(args[1])+1, sizeof(char));
	strcpy(a, args[1]);
	char *ptr;
	strtol(args[1], &ptr, 10);
	if (strlen(ptr) != 0){
		printf("Error: arguments should only be ints\n");
		free(a);
		return;
	}
	int arg1 = atoi(a);
	if(args[2] != NULL){
		if (args[1][0] != '-'){
			printf("Error: first argument must have a '-' in front\n");
			free(a);
			return;
		}
		free(a);
		a = (char*)calloc(strlen(args[2])+1, sizeof(char));
		strcpy(a, args[2]);
		strtol(args[2], &ptr, 10);
		if (strlen(ptr) != 0){
			free(a);
			printf("Error: Incorrect second argument\n");
		}
		else{
			int arg2 = atoi(a);
			arg1 = arg1 * -1;
			free(a);
			if (arg1 == 9){
				freeAll(prompt, commandline, args, pwd, owd, pathHolder, pathlist);
			}
			kill(arg2, arg1);
			perror("Error: ");
		}
		return;
	}
	else{
		free(a);
		freeAll(prompt, commandline, args, pwd, owd, pathHolder, pathlist);
		kill(arg1, SIGTERM);
		perror("Error: ");
		return;
	}
}
void printenv(char **args){
	/*
	  Params: the list of arguments, char** args
	  Description: Will print out all the environmental Variables
	  Returns: void
	*/
	int j = 0;
	if (args[1] == NULL){
		while (environ[j]){
			printf("%s\n", environ[j]);
			j++;
		}
	}
	else if (args[2] != NULL){
		printf("Printenv: Too many arguments\n");
	}
	else{
		char* envpTMP;
		envpTMP = getenv(args[1]);
		if (envpTMP == NULL){
			printf("No matching environemnts were found\n");
		}
		else{
			printf("%s\n", envpTMP);
		}
	}
	return;
}
struct pathelement* setenviron(char **args, struct pathelement *pathlist, char* homedir){
	/*
	  Params: the list of arguments **args, the pathlist which is a linked list of all paths in the environment,
		and the homedirectory which is string (char*).

	  Description: Given 3 or more arguments it will fail. Given 2 arguments it will create a new environmental variable,
		or replace one, named after the 1st argument with the value given in the 2nd argument. Given 1 argument it will create,
		or replace, an environmental variable with the given name and have a value of nothing. Given 0 arguments it will print
		out all the environmental variables like printenv does.

	  Returns: a new pathlist since if the path Environment is changed the pathlist also needs to change and be returned. Its worth
		noting the pathlist is an dynamically allocated linked list.
	*/
	if (args[3] != NULL && args[2] != NULL && args[1] != NULL){
		printf("Setenv: too many arguments\n");
	}
	else if (args[2] != NULL && args[1] != NULL){
		setenv(args[1], args[2], 1);
	}
	else if (args[1] != NULL){
		setenv(args[1], " ", 1);
	}
	else{
		int j = 0;
		while (environ[j]){
			printf("%s\n", environ[j]);
			j++;
		}
		return pathlist;
	}

	if (strcmp(args[1], "PATH") == 0){
		free(pathlist->element);
		struct pathelement *tmp = pathlist;
		while (tmp != NULL){
			struct pathelement *tmp2 = tmp;
			tmp = tmp->next;
			free(tmp2);
		}
		free(tmp);
		pathlist = get_path();
		return pathlist;
	}
	return pathlist;
}
void catcher(){
	//pid_t pid = getpid();
	kill(0, SIGINT);
}
