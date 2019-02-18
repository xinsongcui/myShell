/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#include "command.hh"
#include "shell.hh"

std::string * lastArgument;
Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _infnum = 0;
    _outfnum = 0;
    _append = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    int freeCheck = 0;
	
    // check if there was a double free
    if(_outFile && _errFile){
	if(*_errFile == *_outFile){
		freeCheck = 1;
	}
    }

    _simpleCommands.clear();

    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile && (freeCheck == 0)) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
    _infnum = 0;
    _outfnum = 0;
    _append = 0;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto  *simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void Command::execute() {
    	// Don't do anything if there are no simple commands
	int num = 0;
	if ( _simpleCommands.size() == 0 ) {
       	 	Shell::prompt();
        	return;
    	}
	
	// Mutiple files 
	if( _outfnum >= 2 || _infnum >= 2){
			printf("Ambiguous output redirect.\n");
			clear();
			Shell::prompt();
			return;
	}

        // Print contents of Command data structure
        //print();

        // Add execution here
        // For every simple command fork a new process
        // Setup i/o redirection
        // and call exec

	// save for in/out
	int tmpin = dup(0);
	int tmpout = dup(1);
	int tmperr = dup(2);

	// set initial input
	int fdin;
	int fderr;

	if(_inFile){
		fdin = open(_inFile->c_str(), O_RDONLY);
	}
	else{
		fdin = dup(tmpin);
	}
	
	if(_errFile){
		// Append to file
		if(_append) fderr = open(_errFile->c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
		// No need to apppend	
		else 	fderr = open(_errFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
	}
	else{
		fderr = dup(tmperr);
	}
	dup2(fderr, 2);
	close(fderr);

	int ret;
	int fdout;
	for(unsigned int i = 0; i < _simpleCommands.size(); i++){
		// exit
		if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "exit") == 0){
			close(tmpin);
			close(tmpout);
			close(tmperr);
			close(fdin);
			close(fdout);
			close(fderr);
			printf("Good bye!!\n");
			_exit(1);
		}
		if(Command::_simpleCommands[i]->_arguments.size() != 1 && strcmp(_simpleCommands[i]->_arguments[1]->c_str(), "${?}") == 0){
			printf("%d\n", ex);
			clear();
			close(tmpin);
			close(tmpout);
			close(tmperr);
			Shell::prompt();
			return;
		}
		if(Command::_simpleCommands[i]->_arguments.size() != 1 && strcmp(_simpleCommands[i]->_arguments[1]->c_str(), "${!}") == 0){
			printf("%d\n", pid);
			clear();
			Shell::prompt();
			return;
		}
		if(Command::_simpleCommands[i]->_arguments.size() != 1 && strcmp(_simpleCommands[i]->_arguments[1]->c_str(), "${_}") == 0){
			//printf("cnm");
			std::string * s = new std::string(lastArgument->c_str());
			//printf(s->c_str());
			Command::_simpleCommands[i]->_arguments[1] = s;
			//printf(lastArgument->c_str());
			//clear();
			//Shell::prompt();
			//return;
		}
		// setenv
		if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "setenv") == 0){
			int error = setenv(_simpleCommands[i]->_arguments[1]->c_str(), _simpleCommands[i]->_arguments[2]->c_str(), 1);
			if(error){
				perror("setenv");		
			}				
			clear();
			close(tmpin);
			close(tmpout);
			close(tmperr);
			Shell::prompt();
			return;
		}
		// unsetenv
		if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "unsetenv") == 0){				
			int error = unsetenv(_simpleCommands[i]->_arguments[1]->c_str());
			if(error){
				perror("unsetenv");
			}				
			clear();
			close(tmpin);
			close(tmpout);
			close(tmperr);
			Shell::prompt();
			return;
		}
		// cd
		if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "cd") == 0){
			int error;
			if(_simpleCommands[i]->_arguments.size() == 1){					
				error = chdir(getenv("HOME"));
			}				
			if(_simpleCommands[i]->_arguments.size() > 1){
				error = chdir(_simpleCommands[i]->_arguments[1]->c_str());
			}
			if(error == -1){ 
				std::cerr << "cd: can't cd to " << _simpleCommands[i]->_arguments[1]->c_str();
			}
			clear();
			
			close(tmpin);
			close(tmpout);
			close(tmperr);
			close(fdin);
			
			Shell::prompt();
			return;
		}
		// redirect input
		dup2(fdin, 0);
		close(fdin);
		
		// set output
		if(i == _simpleCommands.size()-1){
			// Last simple command
			if(_outFile){
				// Append to file
				if(_append) fdout = open(_outFile->c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);

				// No need to append
				else fdout = open(_outFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
			}
			else {
				// Use default output
				fdout = dup(tmpout);
			}
		}
		else{
			// Not last simple command
			// create pipe
			int fdpipe[2];
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}

		// Redirect output
		dup2(fdout, 1);
		close(fdout);

		// Create child process
		ret = fork();
		if(ret == -1){
			perror("fork\n");
			exit(2);
		}
		if(ret == 0){
			// exit
			//if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "exit") == 0){
			//	printf("Good bye!!\n");
			//	_exit(1);
			//}
   			// printenv
			if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv") == 0){
				char ** p = environ;
				while(*p!=NULL){
					printf("%s\n", *p);
					p++;
				}
				clear();
				Shell::prompt();
				exit(0);
			}
			// Vector to cstring 
			char** tmpcmd = new char* [_simpleCommands[i]->_arguments.size()+1];
			for(unsigned int j=0; j < _simpleCommands[i]->_arguments.size(); j++){
				tmpcmd[j] = const_cast<char*>(_simpleCommands[i]->_arguments[j]->c_str());
			}
			tmpcmd[_simpleCommands[i]->_arguments.size()] = NULL;
			execvp(_simpleCommands[i]->_arguments[0]->c_str(), tmpcmd);
			perror("execvp");
			_exit(1);
		}else if(_background) pid = ret;
	}
	dup2(tmpin, 0);
	dup2(tmpout, 1);
	dup2(tmperr, 2);
	close(tmpin);
	close(tmpout);
	close(tmperr);
	
	if(!_background){
		// Wait for last command
		waitpid(ret, &num, 0);
		ex = WEXITSTATUS(num);
		//int i = _simpleCommands.size()-1;
		//int j = _simpleCommands[i]->_arguments.size();
		//lastArgument = _simpleCommands[i]->_arguments[j];
	}
	
	int i = _simpleCommands.size()-1;
	int j = _simpleCommands[i]->_arguments.size()-1;
	lastArgument = new std::string( _simpleCommands[i]->_arguments[j]->c_str());
    	// Clear to prepare for next command
    	clear();

    	// Print new prompt
    	Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
