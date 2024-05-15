#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"

int server_socketfd;
int client_socketfd;
int daemon_mode = 0; // flag
pthread_mutex_t mutex;

// linked list for threads
struct slist_thread {
  pthread_t thread;
  int client_socketfd;
  SLIST_ENTRY(slist_thread) entries;
};
SLIST_HEAD(slisthead, slist_thread) head;

void sig_handler(int signo) {
  if (signo == SIGINT || signo == SIGTERM) {
    struct slist_thread *thread_entry = NULL;
    SLIST_FOREACH(thread_entry, &head, entries) {
      if (pthread_cancel(thread_entry->thread) == -1) {
        perror("pthread_cancel");
      }
      if (pthread_join(thread_entry->thread, NULL) != 0) {
        perror("pthread_join");
      }
      free(thread_entry);
    }
    // close(server_socketfd);
    // close(client_socketfd);
    pthread_mutex_destroy(&mutex);
    remove(DATA_FILE);
    syslog(LOG_INFO, "Caught signal, exiting");
    closelog();
    exit(EXIT_SUCCESS);
  }
}

void *add_timestamp(void* arg) {
  while (1) {
    time_t current_time;
    struct tm *time_info;
    char timestamp_str[64];
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(timestamp_str, sizeof(timestamp_str), "timestamp:%a, %d %b %Y %H:%M:%S %z", time_info);
    // open file and write timestamp
    pthread_mutex_lock(&mutex);
    FILE *data_file = fopen(DATA_FILE, "a");
    if (data_file != NULL) {
      // printf("add_timestamp: %d\n", fileno(data_file));
      fprintf(data_file, "%s\n", timestamp_str);
      fclose(data_file);
    }
    pthread_mutex_unlock(&mutex);
    // string should be appended every 10 seconds
    sleep(10);
  }
}

void *connection_handler (void* thread_arg) {
  // printf("did it make it to accept\n");
  struct sockaddr_in client_address;
  socklen_t client_address_size = sizeof(client_address);
  struct slist_thread *thread_entry = (struct slist_thread*)thread_arg;
  // if ((thread_entry->client_socketfd = accept(server_socketfd, (struct sockaddr *)&client_address, &client_address_size)) == -1) {
  if (getpeername(thread_entry->client_socketfd, (struct sockaddr *)&client_address, &client_address_size) == 0) {
    syslog(LOG_INFO, "Accepted connection from %s\n", inet_ntoa(client_address.sin_addr));
  }

  ssize_t bytes_received;
  size_t buffer_size = 1024; // initial buffer size
  char *buffer = malloc(buffer_size); // allocate initial buffer
  if (buffer == NULL) {
    perror("buffer malloc");
    exit(EXIT_FAILURE);
  }

  size_t total_bytes_received = 0;
  while ((bytes_received = recv(thread_entry->client_socketfd, buffer + total_bytes_received, buffer_size - total_bytes_received, 0))) {
    if (bytes_received < 0) {
      perror("recv");
      exit(EXIT_FAILURE);
    }
    total_bytes_received += bytes_received;

    // if buffer is full
    if (total_bytes_received == buffer_size) {
      // double the buffer size
      buffer_size *= 2;
      buffer = realloc(buffer, buffer_size); // resize buffer
      if (buffer == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
      }

      // initialize the newly allocated memory
      memset(buffer + total_bytes_received, 0, buffer_size - total_bytes_received);
    }

    // check for newline to consider packet complete
    char *newline = strchr(buffer, '\n');
    if (newline != NULL) {
      syslog(LOG_INFO, "Received full data packet from %s", inet_ntoa(client_address.sin_addr));
      // write only up to the newline character
      size_t bytes_to_write = newline - buffer + 1; // include the newline character
      pthread_mutex_lock(&mutex);

      printf("Buffer content: ");
      for (size_t i = 0; i < total_bytes_received; ++i) {
        putchar(buffer[i]);
      }
      printf("\n");
      FILE *data_file_a = fopen(DATA_FILE, "a");
      // printf("FIRST connection_handler: %d\n", fileno(data_file));
      if (data_file_a == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
      }
      fwrite(buffer, 1, bytes_to_write, data_file_a);
      fclose(data_file_a);

      // send the data back
      FILE *data_file = fopen(DATA_FILE, "r");
      // printf("SECOND connection_handler: %d\n", fileno(data_file));
      if (data_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
      }

      // calculate file size
      fseek(data_file, 0, SEEK_END);
      long file_size = ftell(data_file);
      rewind(data_file);

      // read content
      char *file_content = malloc(file_size);
      if (file_content == NULL) {
        perror("file_content malloc");
        exit(EXIT_FAILURE);
      }
      fread(file_content, 1, file_size, data_file);
      fclose(data_file);

      // send to client
      printf("Sending content: ");
      for (size_t i = 0; i < file_size; ++i) {
        putchar(file_content[i]);
      }
      printf("\n");
      send(thread_entry->client_socketfd, file_content, file_size, 0);
      free(file_content);
      pthread_mutex_unlock(&mutex);
      // break;
    }
  }

  // return data to client
  // pthread_mutex_lock(&mutex);

  // clean up
  close(thread_entry->client_socketfd);
  // free(thread_entry);
  free(buffer);
  // pthread_mutex_unlock(&mutex);
  syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_address.sin_addr));
  pthread_exit(NULL);
}

void daemonize() {
  pid_t pid;

  // fork off the parent process
  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  // if we got a PID
  if (pid > 0) {
    // exit parent process
    exit(EXIT_SUCCESS);
  }

  // change the file mode mask
  umask(0);

  // create a new SID for the child process
  if (setsid() < 0) {
    perror("setsid");
    exit(EXIT_FAILURE);
  }
  // change the current working directory to root
  if (chdir("/") < 0) {
    perror("chdir");
    exit(EXIT_FAILURE);
  }
  // close the standard file descriptors
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

int main(int argc, char *argv[]) {
  openlog("aesdsocket", LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_LOCAL1);
  int opt;

  // parse cli args
  while ((opt = getopt(argc, argv, "d")) != -1) {
    switch (opt) {
      case 'd':
        daemon_mode = 1;
        break;
      default:
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  // signal handler setup
  struct sigaction sigact;
  sigact.sa_handler = sig_handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);

  // 0. socket step: establishing connection
  server_socketfd = socket(PF_INET, SOCK_STREAM, 0);
  if (server_socketfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
  // handle `bind: Address alredy in use`: https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
  if (setsockopt(server_socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    perror("setsockopt(SO_REUSEADDR)");
    exit(EXIT_FAILURE);
  }

  // 1. bind step: assign address to the socket
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address)); // initialize the struct to zero
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;
  if (bind(server_socketfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)  {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  // 2. listen step: initialize socket for incoming connections
  if (listen(server_socketfd, 5) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_init(&mutex, NULL) != 0) {
    perror("pthread_mutex_init");
    exit(EXIT_FAILURE);
  }

  if (daemon_mode) {
    daemonize(); // convert the server to a daemon
  }

  // 3. accept/handle step:
  SLIST_INIT(&head);
  pthread_t timestamp_thread;
  if (pthread_create(&timestamp_thread, NULL, add_timestamp, NULL) != 0) {
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }
  while (1) {
    struct slist_thread *thread_entry = (struct slist_thread *)malloc(sizeof(struct slist_thread));

    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(struct sockaddr_in);
    // struct slist_thread *thread_entry = (struct slist_thread*)thread_arg;
    if ((thread_entry->client_socketfd = accept(server_socketfd, (struct sockaddr *)&client_address, &client_address_size)) == -1) {
      perror("accept");
      free(thread_entry);
      exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread_entry->thread, NULL, connection_handler, thread_entry) != 0) {
      perror("pthread_create");
      free(thread_entry);
      close(thread_entry->client_socketfd);
      exit(EXIT_FAILURE);
    }
    else {
      SLIST_INSERT_HEAD(&head, thread_entry, entries);
    }
  }
  close(server_socketfd);
  return 0;
}
