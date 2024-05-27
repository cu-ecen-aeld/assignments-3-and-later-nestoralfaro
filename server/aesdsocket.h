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
#include <fcntl.h>

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




//
// #include <pthread.h>
// #include "queue.h"
//
// #define BUFFER_SIZE 128
// #define CONN_PENDING 0
// #define CONN_TERMINATED 1
//
// struct aesdsocketclientconn {
//     int                 data_fd;
//     int                 client_fd;
//     char*               client_ip_addr;
//     pthread_mutex_t*    mutex;
//     size_t              init_buffer_size;
//     char**              buffer;
//     // int*                status;
// };
//
// typedef struct _clientconn_info {
//     SLIST_ENTRY(_clientconn_info) next;
//     pthread_t  thread_id;
//     struct aesdsocketclientconn* ref;
// } clientconn_info;
//
// SLIST_HEAD(conn_list, _clientconn_info) connections;
//
// /**
//  * Accepts server connection by creating a new thread to handle it.
// */
// void accept_conn();
//
// void* connnection_handler(void* param);
//
// /*
//  * Performs connection cleanup upon normal termination or when thread gets cancellation request
//  * Steps:
//  * 1. close client FD
//  * 2. release buffer memory
//  * 3. unlock mutex to allow successful mutex desctruction later
// */
// void connection_cleanup(void* param);
// /*
//  * looping thru LL and cleanup terminated connections and remove them from LL 
// */
// void cleanup_term_conn(int await_termination);
//
