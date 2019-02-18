#include <cstdio>
#include <unistd.h>
#include "shell.hh"
#include "signal.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
int yyparse(void);

void Shell::prompt() {
  if ( isatty(0) ) {
  	printf("myshell>");
  	fflush(stdout);
  }
}

extern "C" void ctrlC( int sig )
{
   printf("\n");
   Shell::prompt();
}

extern "C" void zombie( int sig )
{
   int i = 0;
   while((i = waitpid(0, NULL, WNOHANG))>0) {
	std::string* s= new std::string(" ");
	*s = std::to_string(i);
	setenv("!", s->c_str(), 1);
   }
   //int pid = wait3(0, 0, NULL);
   //while (waitpid(-1, NULL, WNOHANG) > 0);
}
//char* rpath;
int main() {
  //rpath = strdup(argv[0]);
  struct sigaction sa;
  sa.sa_handler = ctrlC;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  int error1 = sigaction(SIGINT, &sa, NULL);
  if(error1){
      perror("sigaction");
      exit(-1);
  }

  struct sigaction sb;
  sb.sa_handler = zombie;
  sigemptyset(&sb.sa_mask);
  sb.sa_flags = SA_RESTART;
  int error2 = sigaction(SIGCHLD, &sb, NULL);
  if(error2){
      perror("sigaction");
      exit(-1);
  }

  Shell::prompt(); 
  yyparse();
}

Command Shell::_currentCommand;
