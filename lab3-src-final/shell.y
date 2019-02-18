

/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
 
}

%token <cpp_string> WORD
%token NOTOKEN GREAT NEWLINE PIPE GREATGREAT AMPERSAND LESS GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT

%{
//#define yylex yylex
#include <cstdio>
#include <vector>
#include <algorithm>
#include <string.h>
#include <dirent.h>
#include <iterator>
#include "shell.hh"

void yyerror(const char * s);
void expandWildcardsIfNecessary(std::string * arg);
bool myfunction(std::string* i, std::string* j);
void expandWildcard(char * prefix, char * suffix);
int yylex();
int check = 1;
%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: simple_command
       ;

simple_command:	
  command_and_args pipe_list iomodifier_list background_optional NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | NEWLINE 

  | error NEWLINE { yyerrok; }
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  |
  ;

argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    expandWildcardsIfNecessary($1);
    //Command::_currentSimpleCommand->insertArgument($1);
  }
  ;

command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument($1);
  }
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | 
  ;

iomodifier_opt:
  GREAT WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._outfnum++;
  }
  | GREATGREAT WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._append = 1;
    Shell::_currentCommand._outfnum++;
  }
  | GREATAMPERSAND WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._errFile = $2;
    Shell::_currentCommand._outfnum++;
  }
  | GREATGREATAMPERSAND WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._errFile = $2;
    Shell::_currentCommand._append = 1;
    Shell::_currentCommand._outfnum++;
  }
  | LESS WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._inFile = $2;
    Shell::_currentCommand._infnum++;
  }
  | TWOGREAT WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._errFile = $2;
  }
  ;

iomodifier_list:
  iomodifier_list iomodifier_opt
  | 
  ;

background_optional:
  AMPERSAND {
    Shell::_currentCommand._background = true;
  }
  |
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

std::vector<std::string *> entries;

void expandWildcardsIfNecessary(std::string * arg){
  if(!strcmp(arg->c_str(), "${?}")){
	Command::_currentSimpleCommand->insertArgument(arg);
	return;
  }
  if(!strchr(arg->c_str(), '?') && !strchr(arg->c_str(), '*')){
	Command::_currentSimpleCommand->insertArgument(arg);
	return;
  }
  // printf("cnm");
  expandWildcard(NULL, const_cast<char*>(arg->c_str()));
  if(entries.size() == 0){
	Command::_currentSimpleCommand->insertArgument(arg);
	return;
  }
  else{
	std::sort(entries.begin(), entries.end(), myfunction);
	for(std::vector<std::string *>::size_type i = 0; i != entries.size(); i++){
		Command::_currentSimpleCommand->insertArgument(entries[i]);
	}
  } 
  entries.clear();
  return;
}

#define MAXFILENAME 1024
void expandWildcard(char * prefix, char * suffix){
  
  char* save = (char*)malloc(MAXFILENAME);
  char* temp = suffix;
  char* dp = save;
 
  if (temp[0] == '/') *(save++) = *(temp++);
  while ((*temp != '/') && (*temp!= NULL)){
	 *(save++) = *(temp++);
  }
  *save = '\0';
   
  /*char * s = strchr(suffix, '/');
  char component[MAXFILENAME];
  if(s!=NULL){
	strncpy(component, suffix, s-suffix);
	suffix = s + 1;
  }  
  else{
	strcpy(component, suffix);
	suffix = suffix + strlen(suffix);
  }
  */
  //printf("cnm");
  char* newPrefix = (char*)malloc(MAXFILENAME);
  if(!strchr(dp, '?') && !strchr(dp, '*')){
	if(prefix!=NULL){
		sprintf(newPrefix, "%s/%s", prefix, dp);
	}
        else {
		newPrefix = strdup(dp);
	}
	if(*temp){	
		temp++;	
		expandWildcard(newPrefix, temp);
	}
	return;
  }
  if(!prefix && suffix[0] == '/'){
	prefix = strdup("/");
        dp++;
  }
  char* reg = (char*)malloc(2*strlen(suffix)+10); 
  char* a = dp;
  char* r = reg;
  *r = '^';
  r++;
  while(*a){
	if (*a == '*'){ *r='.'; r++; *r='*'; r++; }
        else if (*a == '?') { *r='.'; r++; }
	else if (*a =='.') { *r='\\'; r++; *r='.'; r++; }
	else { *r=*a; r++; }
	a++;
  }
  *r='$'; r++; *r=0;

  regex_t re;
  regmatch_t match;
  
  int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
  char * dir;
  if(prefix == NULL){
	dir = ".";
  }
  else dir = prefix;
  DIR * d = opendir(dir);
  if(d == NULL)return;
  //printf("cnm");
  struct dirent * ent;
  while((ent = readdir(d))!=NULL){
	//printf(ent->d_name);
	if(!regexec(&re, ent->d_name, 1, &match, 0)){
		//printf("cnm");
		if(*temp!=NULL){
			if(*temp == '/'){
				temp++;
			}
			if (strcmp(dir, ".") == 0) {
				newPrefix = strdup(ent->d_name);
			}
			else if (strcmp(dir, "/") == 0) {
				sprintf(newPrefix, "%s%s", dir, ent->d_name);
			}
			else {
				sprintf(newPrefix, "%s/%s", dir, ent->d_name);
			}
			expandWildcard(newPrefix, temp);
			//sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
			//expandWildcard(newPrefix, suffix);	
		}
		else{
			//printf(ent->d_name);
			if(prefix!=NULL){
				sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
			}
			else {
				newPrefix = strdup(ent->d_name);
			}
			if(ent->d_name[0] == '.'){
				if(suffix[0] == '.'){
					std::string * s = new std::string(newPrefix);
					entries.push_back(s);
				}
			}
			else{
				std::string * s  = new std::string(newPrefix);
				entries.push_back(s);
			}
			//printf(newPrefix);
		}
	}
	 
  }	
  closedir(d);
}

bool myfunction (std::string * i, std::string* j){
	return *i<*j;
}

#if 0
main()
{
  yyparse();
}
#endif
