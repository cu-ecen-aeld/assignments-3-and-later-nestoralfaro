// #include "aesdsocket.h"
// #include <stdio.h>
// #include <string.h>
//
// int server_socketfd;
// bool daemon_mode = false;
// pthread_mutex_t mutex;
// #if !USE_AESD_CHAR_DEVICE
// pthread_t timestamp_thread;
// #endif
// SLIST_HEAD(slisthead, slist_thread) head;
//
// int main(int argc, char *argv[]) {
//   openlog("aesdsocket", LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_LOCAL1);
//
//   // parse cli args
//   int opt;
//   while ((opt = getopt(argc, argv, "d")) != -1) {
//     switch (opt) {
//       case 'd':
//         daemon_mode = true;
//         break;
//       default:
//         fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
//         exit(EXIT_FAILURE);
//     }
//   }
//
//   // signal handler setup
//   struct sigaction sigact;
//   sigact.sa_handler = sig_handler;
//   sigemptyset(&sigact.sa_mask);
//   sigact.sa_flags = 0;
//   sigaction(SIGINT, &sigact, NULL);
//   sigaction(SIGTERM, &sigact, NULL);
// #if !USE_AESD_CHAR_DEVICE
//   sigaction(SIGALRM, &sigact, NULL);
// #endif
//
//   // 0. socket step: establishing connection
//   server_socketfd = socket(PF_INET, SOCK_STREAM, 0);
//   if (server_socketfd < 0) {
//     perror("socket");
//     exit(EXIT_FAILURE);
//   }
//   // handle `bind: Address alredy in use`: https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
//   if (setsockopt(server_socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
//     perror("setsockopt(SO_REUSEADDR)");
//     exit(EXIT_FAILURE);
//   }
//
//   // 1. bind step: assign address to the socket
//   struct sockaddr_in server_address;
//   memset(&server_address, 0, sizeof(server_address)); // initialize the struct to zero
//   server_address.sin_family = AF_INET;
//   server_address.sin_port = htons(PORT);
//   server_address.sin_addr.s_addr = INADDR_ANY;
//   if (bind(server_socketfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)  {
//     perror("bind");
//     exit(EXIT_FAILURE);
//   }
//
//   // 2. listen step: initialize socket for incoming connections
//   if (listen(server_socketfd, 5) < 0) {
//     perror("listen");
//     exit(EXIT_FAILURE);
//   }
//
//   if (pthread_mutex_init(&mutex, NULL) != 0) {
//     perror("pthread_mutex_init");
//     exit(EXIT_FAILURE);
//   }
//
//   if (daemon_mode) {
//     daemonize(); // convert the server to a daemon
//   }
//
//   SLIST_INIT(&head);
//
//   syslog(LOG_DEBUG, "USE_AESD_CHAR_DEVICE value: %d", USE_AESD_CHAR_DEVICE);
//   #if !USE_AESD_CHAR_DEVICE
//   // if (pthread_create(&timestamp_thread, NULL, add_timestamp, NULL) != 0) {
//   //   perror("timestamp_thread pthread_create");
//   //   exit(EXIT_FAILURE);
//   // }
//   #endif
//
//   // 3. accept/handle step:
//   while (1) {
//     struct slist_thread *thread_entry = (struct slist_thread *)malloc(sizeof(struct slist_thread));
//     if (thread_entry == NULL) {
//       perror("thread_entry malloc");
//       exit(EXIT_FAILURE);
//     }
//     thread_entry->completed = false;
//
//     struct sockaddr_in client_address;
//     socklen_t client_address_size = sizeof(struct sockaddr_in);
//     if ((thread_entry->client_socketfd = accept(server_socketfd, (struct sockaddr *)&client_address, &client_address_size)) == -1) {
//       perror("accept");
//       free(thread_entry);
//       continue;
//     }
//
//     if (pthread_create(&thread_entry->thread, NULL, connection_handler, thread_entry) != 0) {
//       perror("pthread_create");
//       close(thread_entry->client_socketfd);
//       free(thread_entry);
//       continue;
//     }
//     SLIST_INSERT_HEAD(&head, thread_entry, entries);
//
//     struct slist_thread *current_thread_entry;
//     SLIST_FOREACH(current_thread_entry, &head, entries) {
//       if (current_thread_entry->completed) {
//         if (pthread_join(current_thread_entry->thread, NULL) != 0) {
//           perror("SLIST_FOREACH CLEANUP pthread_join current_thread_entry");
//         }
//         SLIST_REMOVE(&head, current_thread_entry, slist_thread, entries);
//         // free(current_thread_entry);
//       }
//     }
//   }
// }
//
// void sig_handler(int signo) {
//   if (signo == SIGINT || signo == SIGTERM) {
//     struct slist_thread *thread_entry;
//     while (!SLIST_EMPTY(&head)) {
//       thread_entry = SLIST_FIRST(&head);
//       if (pthread_cancel(thread_entry->thread) == -1) {
//         perror("pthread_cancel SHUTDOWN thread_entry");
//       }
//       if (pthread_join(thread_entry->thread, NULL) != 0) {
//         perror("pthread_join SHUTDOWN thread_entry");
//       }
//       close(thread_entry->client_socketfd);
//       SLIST_REMOVE_HEAD(&head, entries);
//       // free(thread_entry);
//     }
//
//     #if !USE_AESD_CHAR_DEVICE
//     if (pthread_cancel(timestamp_thread) == -1) {
//       perror("timestamp_thread pthread_cancel");
//     }
//     if (pthread_join(timestamp_thread, NULL) != 0) {
//       perror("timestamp_thread pthread_join");
//     }
//     #endif
//     close(server_socketfd);
//     pthread_mutex_destroy(&mutex);
//     #if !USE_AESD_CHAR_DEVICE
//     remove(DATA_FILE);
//     #endif
//     syslog(LOG_INFO, "Caught signal, exiting");
//     closelog();
//     exit(EXIT_SUCCESS);
//   }
//   #if !USE_AESD_CHAR_DEVICE
//   else if (signo == SIGALRM) {
//     // pthread_mutex_lock(&mutex);
//     // FILE *data_file = fopen(DATA_FILE, "a");
//     // if (data_file != NULL) {
//     //   time_t current_time;
//     //   struct tm *time_info;
//     //   char timestamp_str[64];
//     //   time(&current_time);
//     //   time_info = localtime(&current_time);
//     //   strftime(timestamp_str, sizeof(timestamp_str), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", time_info);
//     //   fprintf(data_file, "%s", timestamp_str);
//     //   fclose(data_file);
//     // }
//     // pthread_mutex_unlock(&mutex);
//   }
//   #endif
// }
//
// void daemonize() {
//   pid_t pid;
//
//   // fork off the parent process
//   pid = fork();
//   if (pid < 0) {
//     perror("fork");
//     exit(EXIT_FAILURE);
//   }
//
//   // if we got a PID
//   if (pid > 0) {
//     // exit parent process
//     exit(EXIT_SUCCESS);
//   }
//
//   //
//   // // change the file mode mask
//   // umask(0);
//   //
//
//   // // create a new SID for the child process
//   // if (setsid() < 0) {
//   //   perror("setsid");
//   //   exit(EXIT_FAILURE);
//   // }
//   // // change the current working directory to root MAKE SURE TO SYNCH with start-stop-aesdsocket.sh
//   // if (chdir("/") < 0) {
//   //   perror("chdir");
//   //   exit(EXIT_FAILURE);
//   // }
//   // // close the standard file descriptors
//   // close(STDIN_FILENO);
//   // close(STDOUT_FILENO);
//   // close(STDERR_FILENO);
// }
//
// #if !USE_AESD_CHAR_DEVICE
// void *add_timestamp(void* arg) {
//   while (1) {
//     time_t current_time;
//     struct tm *time_info;
//     char timestamp_str[64];
//     time(&current_time);
//     time_info = localtime(&current_time);
//     strftime(timestamp_str, sizeof(timestamp_str), "timestamp:%a, %d %b %Y %H:%M:%S %z", time_info);
//     // open file and write timestamp
//     pthread_mutex_lock(&mutex);
//     FILE *data_file = fopen(DATA_FILE, "a");
//     if (data_file != NULL) {
//       fprintf(data_file, "%s\n", timestamp_str);
//       fclose(data_file);
//     }
//     pthread_mutex_unlock(&mutex);
//     // string should be appended every 10 seconds
//     sleep(10);
//   }
// }
// #endif
//
// void *connection_handler (void* thread_arg) {
//   struct slist_thread *thread_entry = (struct slist_thread*)thread_arg;
//   struct sockaddr_in client_address;
//   socklen_t client_address_size = sizeof(client_address);
//   if (getpeername(thread_entry->client_socketfd, (struct sockaddr *)&client_address, &client_address_size) == 0) {
//     syslog(LOG_DEBUG, "USE_AESD_CHAR_DEVICE value: %d", USE_AESD_CHAR_DEVICE);
//     syslog(LOG_INFO, "Accepted connection from %s\n", inet_ntoa(client_address.sin_addr));
//   }
//
//   ssize_t bytes_received;
//   size_t buffer_size = 1024; // initial buffer size
//   char *buffer = malloc(buffer_size); // allocate initial buffer
//   if (buffer == NULL) {
//     perror("buffer malloc");
//     close(thread_entry->client_socketfd);
//     thread_entry->completed = true;
//     exit(EXIT_FAILURE);
//   }
//
//   size_t total_bytes_received = 0;
//
//   while ((bytes_received = recv(thread_entry->client_socketfd, buffer + total_bytes_received, buffer_size - total_bytes_received, 0))) {
//     printf("initial bytes received: %d\n", bytes_received);
//     if (bytes_received < 0) {
//       perror("recv");
//       thread_entry->completed = true;
//       exit(EXIT_FAILURE);
//     }
//     total_bytes_received += bytes_received;
//
//     // if buffer is full
//     if (total_bytes_received == buffer_size) {
//       // double the buffer size
//       buffer_size *= 2;
//       buffer = realloc(buffer, buffer_size); // resize buffer
//       if (buffer == NULL) {
//         perror("realloc");
//         thread_entry->completed = true;
//         exit(EXIT_FAILURE);
//       }
//
//       // initialize the newly allocated memory
//       // memset(buffer + total_bytes_received, 0, buffer_size - total_bytes_received);
//     }
//
//     // check for newline to consider packet complete
//     // char *newline = strchr(buffer, '\n');
//     char *newline = memchr(buffer, '\n', total_bytes_received);
//     if (newline != NULL) {
//       syslog(LOG_INFO, "Received full data packet from %s", inet_ntoa(client_address.sin_addr));
//
//       // write only up to the newline character
//       size_t bytes_to_write = newline - buffer + 1; // include the newline character
//       pthread_mutex_lock(&mutex);
//
//       #if USE_AESD_CHAR_DEVICE
//       int data_file_fd = open(DATA_FILE, O_RDWR);
//       if (data_file_fd == -1) {
//           perror("open");
//           thread_entry->completed = true;
//           pthread_mutex_unlock(&mutex);
//           exit(EXIT_FAILURE);
//       }
//
//       if (write(data_file_fd, buffer, bytes_to_write) == -1) {
//           perror("write");
//           close(data_file_fd);
//           thread_entry->completed = true;
//           pthread_mutex_unlock(&mutex);
//           exit(EXIT_FAILURE);
//       }
//
//       // Seek to the beginning of the file
//       if (lseek(data_file_fd, 0, SEEK_SET) == -1) {
//           perror("lseek SEEK_SET");
//           close(data_file_fd);
//           thread_entry->completed = true;
//           pthread_mutex_unlock(&mutex);
//           exit(EXIT_FAILURE);
//       }
//
//       char readbuf[1024];
//       ssize_t bytes_read;
//
//       // Debug: Loop start
//       printf("Entering read loop\n");
//
//       while ((bytes_read = read(data_file_fd, readbuf, sizeof(readbuf))) > 0) {
//           // Debug: Print bytes read
//           printf("Read %zd bytes\n", bytes_read);
//
//           if (send(thread_entry->client_socketfd, readbuf, bytes_read, 0) == -1) {
//               perror("send");
//               close(data_file_fd);
//               thread_entry->completed = true;
//               pthread_mutex_unlock(&mutex);
//               exit(EXIT_FAILURE);
//           }
//       }
//
//       if (bytes_read == -1) {
//           perror("read");
//           close(data_file_fd);
//           thread_entry->completed = true;
//           pthread_mutex_unlock(&mutex);
//           exit(EXIT_FAILURE);
//       }
//
//       // Debug: Loop end
//       printf("Exiting read loop\n");
//       close(data_file_fd);
//       // total_bytes_received = 0;
//       #else
//       FILE *data_file_a = fopen(DATA_FILE, "a");
//       if (data_file_a == NULL) {
//         perror("fopen");
//         thread_entry->completed = true;
//         pthread_mutex_unlock(&mutex);
//         exit(EXIT_FAILURE);
//       }
//       fwrite(buffer, 1, bytes_to_write, data_file_a);
//       fclose(data_file_a);
//
//       // send the data back
//       FILE *data_file = fopen(DATA_FILE, "r");
//       if (data_file == NULL) {
//         perror("fopen");
//         thread_entry->completed = true;
//         pthread_mutex_unlock(&mutex);
//         exit(EXIT_FAILURE);
//       }
//
//       // calculate file size
//       fseek(data_file, 0, SEEK_END);
//       long file_size = ftell(data_file);
//       rewind(data_file);
//
//       // read content
//       char *file_content = malloc(file_size);
//       if (file_content == NULL) {
//         perror("ln 353 file_content malloc");
//         thread_entry->completed = true;
//         fclose(data_file);
//         pthread_mutex_unlock(&mutex);
//         exit(EXIT_FAILURE);
//       }
//       fread(file_content, 1, file_size, data_file);
//       fclose(data_file);
//
//       // send to client
//       send(thread_entry->client_socketfd, file_content, file_size, 0);
//       free(file_content);
//       #endif
//       pthread_mutex_unlock(&mutex);
//       total_bytes_received = 0; // reset for the next packet
//     }
//   }
//
//   // clean up
//   close(thread_entry->client_socketfd);
//   free(buffer);
//   thread_entry->completed = true;
//   syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_address.sin_addr));
// }









#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include "aesdsocket.h"
#include "datafile.h"

#define SERVER_PORT "9000"
#define LISTEN_BACKLOG 10
#define NEWLINE '\n'
#define ISO_2822_TIME_FMT "%a, %d %b %Y %T %z"

int server_fd;
timer_t timer_id;
pthread_mutex_t mutex;
volatile sig_atomic_t stopApp;

void cleanup() {
    if (server_fd > 0) {
        close(server_fd);
    }

    destroy_datafile();
    closelog();
}

static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        stopApp = 1;
        syslog(LOG_DEBUG, "%s", "Caught signal, exiting");

        if (server_fd > 0) {
            while (shutdown(server_fd, SHUT_RDWR) == -1) {
                syslog(LOG_ERR, "Server socket shutdown error: %s", strerror(errno));
            }
            close(server_fd);
        }
        // delete timer
        if (timer_id && timer_delete(timer_id) != 0) {
            syslog(LOG_ERR, "Failure to delete timer: %s", strerror(errno));
        }

        // looping thru connections and terminate threads
        clientconn_info *conn;
        SLIST_FOREACH(conn, &connections, next) {
            pthread_cancel(conn->thread_id);
        }
        cleanup_term_conn(1);
        pthread_mutex_destroy(&mutex);

        destroy_datafile();
        closelog();
    }
}

void make_daemon() {
    // we can use daemon syscall or use two-fork approach
    int pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) < 0) {
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // file perm
    umask(0);
    // change dir /
    chdir("/");
    // close all open file descriptors
    // for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
    //     close(x);
    // }
}

int send_response(int data_fd, int client_fd) {
    char readbuf[1024*100];
    int m;
    // position to the beginning
    adjust_datafile_pos(data_fd, 0, SEEK_SET);

    while((m = read(data_fd, readbuf, sizeof(readbuf))) > 0) {
        int rc = send(client_fd, readbuf, m, 0);
        if (rc < 0) {
            return rc;
        }
    }

    return 0;
}

void* connnection_handler(void* param) {
    struct aesdsocketclientconn* args = (struct aesdsocketclientconn *) param;

    pthread_cleanup_push(connection_cleanup, args);

    syslog(LOG_DEBUG, "Accepted connection from %s", args->client_ip_addr);

    size_t maxbuflen = args->init_buffer_size;
    char recv_buf[BUFFER_SIZE];
    int n;
    size_t buflen = 0;
    *args->buffer[0] = '\0';

    while((n = recv(args->client_fd, recv_buf, sizeof(recv_buf), 0)) > 0) {
        int data_fd = open_datafile();
        args->data_fd = data_fd;
        if (adjust_datafile_pos(data_fd, 0, SEEK_END) > -1) {
            // locate position of newline
            int k = 0;
            int has_nl = 0;
            for (; k < n; k++) if (recv_buf[k] == NEWLINE) {
                has_nl = 1;
                break;
            }
            int currlen = n;
            if (k+1 < n) {
                currlen = k+1;
            }

            append_string(args->buffer, &maxbuflen, buflen, recv_buf, currlen);
            buflen += currlen;

            if (has_nl) {
                int rc = pthread_mutex_lock(args->mutex);
                if (rc != 0) {
                    syslog(LOG_ERR, "Failure to lock mutex with error code: %d", rc);
                    break;
                }
                // append to the data file
                append_datafile(data_fd, *args->buffer, strlen(*args->buffer));
                *args->buffer[0] = '\0'; // make string empty!
                buflen = 0;

                rc = send_response(data_fd, args->client_fd);
                if (rc < 0) {
                    syslog(LOG_ERR, "Failure to send response to the client - %s", args->client_ip_addr);
                    break;
                }
                if ((rc = pthread_mutex_unlock(args->mutex)) != 0) {
                    syslog(LOG_ERR, "Failure to unlock mutex. Error code: %d", rc);
                    break;
                }
                // data file position will be EOF and we can copy any bytes after NEWLINE
                if (k < n) {
                    buflen = n-k-1;
                    append_string(args->buffer, &maxbuflen, buflen, &recv_buf[k+1], buflen);
                }
            }
        } else {
            syslog(LOG_ERR, "Failure to reposition cursor in the data file %d to the EOF", data_fd);
            break;
        }
    }

    pthread_cleanup_pop(1);

    return args;
}

void connection_cleanup(void* param) {
    struct aesdsocketclientconn* args = (struct aesdsocketclientconn *)param;
    
    close(args->client_fd);
    syslog(LOG_DEBUG, "Closed connection from %s", args->client_ip_addr);
    if (args->data_fd) {
        close_datafile(args->data_fd);
    }
    free(*args->buffer);
    free(args->buffer);
    free(args->client_ip_addr);
    pthread_mutex_unlock(args->mutex); // don't need to handle failures since mutex is PTHREAD_MUTEX_ERRORCHECK
}

void accept_conn() {
    struct sockaddr client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, &client_addr, &client_addr_len);
    if (client_fd == -1) {
        syslog(LOG_ERR, "Failed to accept connection: %s", strerror(errno));
        return;
    }
    // get client ip
    struct sockaddr_in *client_inaddr = (struct sockaddr_in *) &client_addr;
    char *client_ip_addr = inet_ntoa(client_inaddr->sin_addr);

    pthread_t thread_id;
    struct aesdsocketclientconn* conn_data = malloc(sizeof *conn_data);
    conn_data->mutex = &mutex;
    conn_data->client_fd = client_fd;
    conn_data->client_ip_addr = malloc(strlen(client_ip_addr)+1);
    strcpy(conn_data->client_ip_addr, client_ip_addr);
    conn_data->data_fd = 0;
    conn_data->init_buffer_size = BUFFER_SIZE;
    conn_data->buffer = malloc(sizeof(char *));
    *conn_data->buffer = malloc(BUFFER_SIZE);

    int rc = pthread_create(&thread_id, NULL, connnection_handler, conn_data);
    if (rc != 0) {
        syslog(LOG_ERR, "Failure to create thread. Error code: %d", rc);        
        free(conn_data);
    } else {
        // track thread for later monitoring
        clientconn_info* conn = malloc(sizeof(*conn));
        conn->thread_id = thread_id;
        conn->ref = conn_data;
        // conn->conn_status = conn_data->status;
        SLIST_INSERT_HEAD(&connections, conn, next);
        // look thru LL and determine if any connections has ended
        cleanup_term_conn(0);
    }
}

void cleanup_term_conn(int await_termination) {
    clientconn_info *item, *tmp_item;

    SLIST_FOREACH_SAFE(item, &connections, next, tmp_item) {
        // try join
        int rc;
        if (await_termination != 0) {
            rc = pthread_join(item->thread_id, NULL);
        } else {
            rc = pthread_tryjoin_np(item->thread_id, NULL);
        }
        if (rc == 0) {
            SLIST_REMOVE(&connections, item, _clientconn_info, next);
            free(item->ref);
            free(item);
        }
    }
}

int register_sighandlers() {
    struct sigaction app_action;
    memset(&app_action, 0, sizeof(sigaction));
    app_action.sa_handler = signal_handler;
    sigemptyset (&app_action.sa_mask);
    app_action.sa_flags = 0;

    if (sigaction(SIGTERM, &app_action, NULL) != 0) {
        perror("Failure to register SIGTERM handler");
        return -1;
    }
    if (sigaction(SIGINT, &app_action, NULL) != 0) {
        perror("Failure to register SIGINT handler");
        return -1;
    }

    return 0;
}

int start_server(const char *port, int as_daemon) {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "%s", "Failure to open socket");
        cleanup();
        exit(EXIT_FAILURE);
    }
    // set SO_REUSEADDR
    const int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        syslog(LOG_ERR, "Failed to set socket options - %s", "SO_REUSEADDR");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // get addr
    struct addrinfo *addr;
    struct addrinfo hints;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(NULL, port, &hints, &addr);
    if (rc != 0) {
        syslog(LOG_ERR, "Failure to obtain address: return code = %d", rc);
        cleanup();
        exit(EXIT_FAILURE);
    }

    rc = bind(server_fd, addr->ai_addr, sizeof(struct sockaddr));
    freeaddrinfo(addr);
    if (rc != 0) {
        syslog(LOG_ERR, "Unable to bind server on port %s: %s", SERVER_PORT, strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }
    
    if (as_daemon) make_daemon();

    rc = listen(server_fd, LISTEN_BACKLOG);
    if (rc != 0) {            
        syslog(LOG_ERR, "Failure to listen socket: %s", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }
    syslog(LOG_DEBUG, "Listening for connections on port %s", SERVER_PORT);

    return server_fd;
}

static void timer_action(union sigval arg) {
    int rc = pthread_mutex_lock(&mutex);
    if (rc != 0) {
        syslog(LOG_ERR, "Error locking thread data: %s", strerror(errno));
    } else {
        struct timespec ts;
        rc = clock_gettime(CLOCK_REALTIME, &ts);
        if (rc != 0) {
            syslog(LOG_ERR, "Failure to get system wall clock time: %s", strerror(errno));
        } else {
            char dt[100], ts_row[120];
            struct tm tm;
            tzset();
            localtime_r(&ts.tv_sec, &tm);
            size_t len = strftime(dt, 100, ISO_2822_TIME_FMT, &tm);
            strcpy(ts_row, "timestamp:");
            strncat(ts_row, dt, len);
            len = strlen(ts_row);
            ts_row[len] = NEWLINE;
            ts_row[len+1] = '\0';

            int fd = open_datafile();
            if (adjust_datafile_pos(fd, 0, SEEK_END) > -1) 
                append_datafile(fd, ts_row, strlen(ts_row));
            else
                syslog(LOG_ERR, "Failure to adjust data file to position to the end!");
            close_datafile(fd);
        }
        if (pthread_mutex_unlock(&mutex) != 0) {
            syslog(LOG_ERR, "Failed to unlock thread data: %s", strerror(errno));
        }
    }
}

void create_timer() {
    struct sigevent sev;
    memset(&sev, 0, sizeof(struct sigevent));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_action;

    if (timer_create(CLOCK_MONOTONIC, &sev, &timer_id) != 0) {
        syslog(LOG_ERR, "Failure to create timer: %s", strerror(errno));
    } else {
        struct itimerspec ts;
        ts.it_interval.tv_sec = 10;
        ts.it_interval.tv_nsec = 0;
        ts.it_value.tv_sec = 10;
        ts.it_value.tv_nsec = 0;

        if (timer_settime(timer_id, 0, &ts, NULL) != 0) {
            syslog(LOG_ERR, "Failure to arm timer!!!");
        }
    }
}

int main(int argc, char **argv) {
    int daemon = 0;
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon = 1;
    } 

    openlog(NULL, LOG_ODELAY, LOG_USER);

    server_fd = start_server(SERVER_PORT, daemon);
    if (server_fd < 0) {
        return EXIT_FAILURE;
    }

    if (register_sighandlers() != 0) {
        exit(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "Value of flag - %d", USE_AESD_CHAR_DEVICE);
    
    // open and close data file
    int fd = open_datafile();
    if (fd < 0) {
        exit(EXIT_FAILURE);
    }
    close_datafile(fd);

    SLIST_INIT(&connections);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    if (!USE_AESD_CHAR_DEVICE) {
        create_timer();
    }

    for( ; stopApp < 1 ; ) {
        accept_conn();
    }

}
