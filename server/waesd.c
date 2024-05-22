#include "waesd.h"

#define BUF_SIZE 500

bool caught_a_signal = false;
bool caught_sigint = false;
bool caught_sigterm = false;
bool caught_sigalrm = false;

bool should_continue = true;



static void signal_handler(int signal_number) {
    if (signal_number == SIGINT) {
          caught_sigint = true;
          caught_a_signal = true;
          should_continue = false;
    
      }
    if (signal_number == SIGTERM) {
          caught_sigterm = true;
          caught_a_signal = true;
          should_continue = false;
      }
    if (signal_number == SIGALRM) {
    caught_sigalrm = true;
      }
}

void set_signal_handling() {
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler; 
    int sa = sigaction(SIGINT, &new_action, NULL);
    if (sa != 0) {

          exit(-1);
      }
    sa = sigaction(SIGTERM, &new_action, NULL);
    if (sa != 0) {
          syslog(LOG_ERR, "Failed to register for SIGTERM. Error: %d", sa);
    exit(-1);
      }
    sa = sigaction(SIGALRM, &new_action, NULL);
    if (sa != 0) {
          syslog(LOG_ERR, "Failed to register for SIGALRM. Error: %d", sa);
          exit(-1);
      }

    syslog(LOG_INFO, "Signal Handling set up complete.");
    printf("Signal handling set up.\n");
}

void ten_second_timer(pthread_mutex_t *mutex) {
    time_t t;
    struct tm *tmp;
    char outstr[200];
    int r;
    int num_char;
    char header[] = "timestamp:";
    
    struct itimerval timer_spec;

    timer_spec.it_interval.tv_sec = 10;
    timer_spec.it_interval.tv_usec = 0;
    timer_spec.it_value.tv_sec = 10;
    timer_spec.it_value.tv_usec = 0;
    
    r = setitimer(ITIMER_REAL, &timer_spec, 0);
    if (r != 0 ) {
          syslog(LOG_ERR, "Failed to create timer. Error: %d", errno);
    printf("Failed to create timer. Error: %d\n", errno);
    exit(-1);
      }
    /**/
      printf("In timer. Alarm should be set. Should continue: %d\n", should_continue);
    /**/
      while(should_continue) {
    /**/
    sleep(0.01);
          if(caught_sigalrm) {
      printf("Sigalarm caught. Printing timestamp.\n");
      // get and format local wall time
      t = time(NULL);
      tmp = localtime(&t);
      if (tmp == NULL) {
    syslog(LOG_ERR, "Failed to get local time. Error: %d", errno);
          printf("Failed to get local time. Error: %d", errno);
          exit(-1);
      }
            
            num_char = strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp);
            if (num_char == 0) {
          syslog(LOG_ERR, "Failed to format time. Returned 0.");
    printf("Failed to format time. Returned 0.");
    exit(-1);
      }
      outstr[num_char] = '\n';
      
            // write timestamp to file
      r = pthread_mutex_lock(mutex);
      printf("Printing timestamp: %s", outstr);
      int output_file = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (output_file < 0) {
                syslog(LOG_ERR, "Could not open file. Error: %d", errno);
          printf("Could not open file. Error: %d\n", errno);
    close(output_file);
                pthread_mutex_unlock(mutex);
          exit(-1);
            }
            r = write(output_file, header, sizeof(header));
            r = write(output_file, outstr, num_char + 1);
            if (r < 0) {
    syslog(LOG_ERR, "Could not print to file. Error: %d", r);
    printf("Could not print to file. Error: %d\n", r);
          close(output_file);
                pthread_mutex_unlock(mutex);
    exit(-1);
            }
            pthread_mutex_unlock(mutex);
      caught_sigalrm = false;
        }
    }
    printf("Exiting Timer Process. . . \n");
    exit(0);
}

int open_socket() {
    struct addrinfo hints;
    struct addrinfo *serverinfo;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_addr = NULL;

    int r = getaddrinfo(NULL, PORT, &hints, &serverinfo);
    if (r != 0) {
          syslog(LOG_ERR, "getaddrinfo encountered an error: %d", r);
          freeaddrinfo(serverinfo);
          exit(-1);
      }

    int sfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
      if (sfd == -1) {
          syslog(LOG_ERR, "Socket could not be created. Error: %d", errno);
          freeaddrinfo(serverinfo);
          exit(-1);
      }
    const int opt_yes = 1;
      r = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt_yes, sizeof(opt_yes));
    if (r == -1) {
          syslog(LOG_ERR, "Socket options could not be set. Error: %d", errno);
          freeaddrinfo(serverinfo);
          exit(-1);
      }

    r = bind(sfd, serverinfo->ai_addr, serverinfo->ai_addrlen);
    if (r != 0) {
          syslog(LOG_ERR, "Socket failed to bind. Error: %d", errno);
          freeaddrinfo(serverinfo);
          exit(-1);
      }
    freeaddrinfo(serverinfo);
    return sfd;
}

void* accept_connection_thread(void* thread_param) {
    // accept connection

    struct thread_data* data = (struct thread_data *) thread_param;
    int pfd = data->pfd;

    char client[INET6_ADDRSTRLEN];
    inet_ntop(data->peer.sa_family, data->peer.sa_data, client, sizeof(client));
    syslog(LOG_INFO, "Accepted connection from %s", client);
    printf("Accepted connection from %s\n", client);

    printf("Locking mutex\n");
    int res = pthread_mutex_lock(data->mutex);
    if (res != 0) {
          syslog(LOG_ERR, "pthread mutex lock failed. Code: %d", res);
    exit(-1);
      }
    res = recieve_socket_data(pfd);
    if (res == -1) {
          res = pthread_mutex_unlock(data->mutex);
          exit(-1);
      }
    res = return_socket_data(pfd);
    if (res == -1) {
          res = pthread_mutex_unlock(data->mutex);
    exit(-1);
      }

    printf("Unlocking mutex\n");
    res = pthread_mutex_unlock(data->mutex);
    if (res != 0) {
          syslog(LOG_ERR, "pthread mutex lock failed. Code: %d", res);
    exit(-1);
      }

    syslog(LOG_INFO, "Closing connection at %s", client);
    printf("Closing connection at %s\n", client);
    shutdown(pfd,SHUT_RDWR);
    close(pfd);
    // exit thread upon successful completion
      data->thread_complete_success = true;
}

int recieve_socket_data(int sockfd) {
    char buf[BUF_SIZE+1];    
    ssize_t nrecv;
    ssize_t nwrit;
    char *ptr_null;
    
    int output_file = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (output_file < 0) {
          syslog(LOG_ERR, "Could not open file. Error: %d", errno);
    close(output_file);
    return -1;
      }

    do {
        nrecv = recv(sockfd, buf, BUF_SIZE, 0);
  buf[nrecv] = 0;
  if (nrecv < 0) {
              syslog(LOG_ERR, "Could not recieve data. Error: %d", errno);
              close(output_file);
        return -1;
          }

  nwrit = write(output_file, buf, nrecv);
        if (nwrit < 0) {
        close(output_file);
        printf("Are we erroring here?\n");
              return -1;
          }
  ptr_null = strchr(buf, '\n');
    } while (ptr_null == NULL);

      close(output_file);
    return 0;
}

int return_socket_data(int sockfd) {
    char buf[BUF_SIZE];    
    ssize_t nread;
    ssize_t nsend;

    int output_file = open(OUTPUT_FILE, O_RDONLY, 0666);
    if (output_file < 0) {
          syslog(LOG_ERR, "Could not open file. Error: %d", errno);
    return -1;
      }

    do {    
        nread = read(output_file, buf, BUF_SIZE);
  if (nread < 0) {
        close(output_file);
        return -1;
          }
  nsend = send(sockfd, buf, nread, 0);
        if (nsend < 0) {
              syslog(LOG_ERR, "Could not send data. Error: %d", errno);
        close(output_file);
              return -1;
          }
    } while (nsend == BUF_SIZE);

      close(output_file);
    return 0;
}

void graceful_socket_shutdown(int sockfd, struct slisthead *head) {

    printf("Shutting down aesdsocket.\n");

    // exit all threads
    slist_data_t *datap=NULL;
    datap = malloc(sizeof(slist_data_t));
    int r;
    while (!SLIST_EMPTY(head)) {
          datap = SLIST_FIRST(head);
    r = pthread_cancel(datap->thread_id);
    if (r != 0) {
    syslog(LOG_ERR, "Thread %ld could not be canceled. Code: %d", datap->thread_id, r);
  }
        SLIST_REMOVE_HEAD(head, entries);
    }     
    free(datap);

    r = shutdown(sockfd,SHUT_RDWR);
    if (r != 0) {
          syslog(LOG_ERR, "Socket shutdown failed. Error: %d", errno);
      }
    r = close(sockfd);
    if (r != 0) {
          syslog(LOG_ERR, "Socket close failed. Error: %d", errno);
      }
    remove(OUTPUT_FILE);
}

/* 
 * --------- MAIN --------- 
 */
int main (int argc, char*argv[]) {

    printf("Beginning aesdsocket!\n");
    openlog(NULL, 0, LOG_USER);    

    set_signal_handling();

    // set up thread linked list
    // Adapted from https://github.com/stockrt/queue.h/blob/master/sample.c
    slist_data_t *datap=NULL;
    slist_data_t *datap_temp=NULL;
    struct slisthead head;
    SLIST_INIT(&head);

    // create output file mutex
    pthread_mutex_t mutex;
    int r = pthread_mutex_init(&mutex, NULL);
    if (r != 0) {
          syslog(LOG_ERR, "Error initializing mutex: %d", r);
          exit(-1);
      }

    char buf[BUF_SIZE];
      size_t nread;
      
      int sfd = open_socket();

      if (argc >= 2 && !strcmp(argv[1], "-d")) {
    r = fork();
    if (r < 0) {
            syslog(LOG_ERR, "Error creating deamon fork: %d", errno);
            printf("Error creating deamon fork: %d\n", errno);
      exit(-1);
  } else if (r > 0) {
      exit(0);
  }
    }
    
    int counter = 0;
      bool start_timer = true;
      do {  
        // listen for and accept connection
  int r = listen(sfd, 10);
  if (r != 0) {
        syslog(LOG_ERR, "Error while listening on socket: %d", errno);
        printf("Error while listening on socket: %d\n", errno);
        graceful_socket_shutdown(sfd, &head);
        exit(-1);
    }
        struct sockaddr peer;
          socklen_t peer_addr_size = sizeof(peer);
          memset(&peer, 0, sizeof(peer));   //make sure struct is empty

        printf("Waiting for connection . . .\n");

  int pfd = accept(sfd, &peer, &peer_addr_size); 
    if (pfd == -1) {
        if (errno == 4) {
                break;
      }
      syslog(LOG_ERR, "Error while accepting connection: %d", errno);
      printf("Error while accepting connection: %d\n", errno);
      graceful_socket_shutdown(sfd, &head);
      exit(-1);
  } else {
        pthread_t thread_id;
        struct thread_data *data = (thread_data*)malloc(sizeof(thread_data));
        data->mutex = &mutex;
        data->thread_complete_success = false;
        data->peer = peer;
        data->pfd = pfd;

              int res = pthread_create(&thread_id, NULL, accept_connection_thread, data);
              if (res != 0) {
    syslog(LOG_ERR, "Error while creating thread: %d", res);
    printf("Error while creating thread: %d\n", res);
    graceful_socket_shutdown(sfd, &head);
    free(data);
                exit(-1);
            }
      syslog(LOG_INFO, "Created thread %ld", thread_id);
      printf("Created thread %ld\n", thread_id);

      // Add this thread to the linked list
      // Check the linked list and join any threads that have completed
      // Adapted from https://github.com/stockrt/queue.h/blob/master/sample.c
      datap = malloc(sizeof(slist_data_t));
      datap_temp = malloc(sizeof(slist_data_t));
      datap->thread_id = thread_id;
      datap->tdata = data;
      SLIST_INSERT_HEAD(&head, datap, entries);
      counter++;

      if(counter > 1) {
      SLIST_FOREACH_SAFE(datap, &head, entries, datap_temp) {
                if (datap->tdata->thread_complete_success = true) {
        pthread_join(datap->thread_id, NULL);
        syslog(LOG_INFO, "Joined thread %ld", datap->thread_id);
        printf("Joined thread %ld\n", datap->thread_id);
        SLIST_REMOVE(&head, datap, slist_data_s, entries);
    }
            }
      free(datap);
      free(datap_temp);
      }
  }   
  // after the first connection is made, set up the timer.
    if (start_timer) {
              // set up timer process
              printf("Creating timer . . .\n");
              r = fork();
              if (r < 0) {
                syslog(LOG_ERR, "Error creating timer fork: %d", errno);
                printf("Error creating timer fork: %d\n", errno);
          exit(-1);
            } else if (r == 0) {
          ten_second_timer(&mutex);
            }
      start_timer = false;
  }
    } while(!caught_a_signal);

      if (caught_sigint || caught_sigterm) {
    syslog(LOG_INFO, "Caught signal, exiting.");
    printf("Caught signal, exiting.\n");
          graceful_socket_shutdown(sfd, &head);
          remove(OUTPUT_FILE);
    
      }
    exit(0);
}
