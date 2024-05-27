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
#include <pthread.h>
#include <sys/queue.h>
#include <stdbool.h>

#define PORT 9000

#ifndef USE_AESD_CHAR_DEVICE
# define USE_AESD_CHAR_DEVICE 1
#endif

#if USE_AESD_CHAR_DEVICE
# define DATA_FILE "/dev/aesdchar"
#else
# define DATA_FILE "/var/tmp/aesdsocketdata"
#endif

// linked list for threads
struct slist_thread {
  pthread_t thread;
  int client_socketfd;
  bool completed;
  SLIST_ENTRY(slist_thread) entries;
};

void sig_handler(int signo);
void daemonize();
#if !USE_AESD_CHAR_DEVICE
void *add_timestamp(void* arg);
#endif
void *connection_handler (void* thread_arg);
