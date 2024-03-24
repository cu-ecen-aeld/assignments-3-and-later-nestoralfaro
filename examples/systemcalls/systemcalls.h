#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

// needed for the system calls
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

bool do_system(const char *command);
bool do_exec(int count, ...);
bool do_exec_redirect(const char *outputfile, int count, ...);
