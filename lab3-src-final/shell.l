
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{
#include <sys/wait.h>
#include <cstring>
#include <unistd.h>
#include <pwd.h>
#include "y.tab.hh"

extern "C" char * read_line();

int mygetc(FILE * f){
  static char* p;
  char ch;

  if(!isatty(0)){
	return getc(f);
  }
  
  if(p==NULL || *p == 0){
	char* s = read_line();
	p = s; 
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" {
  return GREATGREAT;
}

"2>" {
  return TWOGREAT;
}

"|" {
  return PIPE;
}

"&" {
  return AMPERSAND;
}

"<" {
  return LESS;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

["][^\n\"]*["] {
  /* quote */
  yylval.string_val = strdup(yytext+1);
  yylval.string_val[yyleng-2] = 0;
  yylval.cpp_string = new std::string(yylval.string_val);
  return WORD;	
}

[~][^ \n\t]* {
  /* tilde */
  char* command = strdup(yytext);
  // printf(command);
  // printf("\n");
  
  if(strlen(command) > 1){
	if(command[1] == '/'){
		char* dir = strdup(getenv("HOME"));
		command++;
		command = strcat(dir, command);
	}
	else{
		command++;
		char* username = (char*)malloc(50);
		char* temp = username;
		
		while((*command != '/') && (*command != NULL)){
			*(username++) = *(command++);
			*username = 0;
		}
		command = strcat(getpwnam(temp)->pw_dir, command);  
	}
	yylval.cpp_string = new std::string(command);
  }
  else	{
	yylval.cpp_string = new std::string(getenv("HOME"));
  }
  return WORD;
}
\$\{\$\} {
  int ret = getpid();
  std::string s = std::to_string(ret);
  yylval.cpp_string = new std::string(s);
  return WORD;
}
\$\{\!\} {
  getenv("HOME");
  while(wait(0)<=0);
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
\$\{\_\} {
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
\$\{\?\} {
  //printf("%d\n", ex);
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

\$\{"SHELL"\} {
  //printf("cnm");
  
  //getcwd(buffer, sizeof(buffer));
  //strcpy(buffer, $1);
  //readlink("/lab3-src/shell", buffer, 100);
  //printf(buffer);
  char* buf = (char*)malloc(2000);
  realpath("../shell", buf);

  //realpath(rpath, buf);
  yylval.cpp_string = new std::string(buf);
  return WORD;

}

\$\{[^ \t\n]*\}[^ \t\n]* {
  char* command = strdup(yytext);
  std::string temp;
  std::string extended;
  
  while(*command){
	if(*command == '$' && *(++command) == '{'){
		while(*command != '}'){
			if(*command != '{') temp += *command;
			command++;
		}
		char* env = getenv(temp.c_str());
		extended += env;
		temp.clear();
		command++;
	}
        if(*command){
		extended += *command;
		command++;
	}
  }
  yylval.cpp_string = new std::string(extended);
  return WORD;
}

\$\([^\n]*\) {
  /* subshell */
  char* command = strdup(yytext+2);
  char tempc;
  command[yyleng-3] = '\0';

  int tmpin = dup(0);
  int tmpout = dup(1);

  int pin[2];
  int pout[2];

  int ret; 

  pipe(pin);
  pipe(pout);

  write(pin[1], command, strlen(command));
  write(pin[1], "\nexit\n", 6);
  close(pin[1]);

  dup2(pin[0], 0);
  close(pin[0]);
  dup2(pout[1], 1);
  close(pout[1]);

  ret = fork();
  if(ret == -1){
	perror("fork\n");
	exit(2);
  }
  if (ret == 0) {
	execvp("/proc/self/exe", NULL);
	_exit(1);
  } 
  dup2(tmpin, 0);
  dup2(tmpout, 1);
  close(tmpin);
  close(tmpout);

  std::string buffer;
	
  while (read(pout[0], &tempc, 1)) {
	if (tempc == '\n') buffer += ' ';
	else {
		buffer += tempc;
	}
  }
  for (std::string::iterator i=buffer.end()-1; i>= buffer.begin(); i--) {
	unput(*i);
  }
}

"source "[^ \t\n]* {
  /* source */
  char* buffer = strdup(yytext+7);
  FILE * yyin = fopen(buffer, "r");
  YY_BUFFER_STATE yybuffer = yy_create_buffer(yyin, YY_BUF_SIZE);
  yypush_buffer_state(yybuffer);
  yyparse();
  yypop_buffer_state();
  fclose(yyin);
}

[^ \t\n]*\\[^ \t\n]* {
  /* escape */
  std::string ntext;	
  std::string text = yytext;
	 
  for(int i=0; i<text.length(); i++){
	if(text[i] != '\\'){
		ntext += text[i];
	}
	if(text[i] == '\\'){
		if(text[i+1] == '\\'){
			ntext += '\\';
			i++;
		}
	}
   }
   yylval.cpp_string = new std::string(ntext);
   return WORD;
}

[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

