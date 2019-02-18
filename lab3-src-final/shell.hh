#ifndef shell_hh
#define shell_hh

#include "command.hh"
#include <regex.h>
#include <stdlib.h>
#include <sys/types.h>

struct Shell {

  static void prompt();

  static Command _currentCommand;
};
inline int pid;
#endif
