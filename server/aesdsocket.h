#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <getopt.h> // for cli args parsing
#include <pthread.h>
#include <sys/queue.h>
#include <stdbool.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"

void daemonize();
void *add_timestamp(void* arg);
void *connection_handler (void* thread_arg);
void sig_handler(int signo);
