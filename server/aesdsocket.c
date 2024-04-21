#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <getopt.h> // for cli args parsing

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"

int server_socketfd;
int client_socketfd;
int daemon_mode = 0; // flag

void sig_handler(int signo) {
  if (signo == SIGINT || signo == SIGTERM) {
    syslog(LOG_INFO, "Caught signal, exiting");
    close(server_socketfd);
    close(client_socketfd);
    remove(DATA_FILE);
    exit(EXIT_SUCCESS);
  }
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
  openlog("aesdsocket", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
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

  if (daemon_mode) {
    daemonize(); // convert the server to a daemon
  }

  // 3. accept/handle step:
  while (1) {
    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(struct sockaddr_in);
    client_socketfd = accept(server_socketfd, (struct sockaddr *)&client_address, &client_address_size);
    if (client_socketfd < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_address.sin_addr));

    FILE *data_file = fopen(DATA_FILE, "a");
    if (data_file == NULL) {
      perror("fopen");
      exit(EXIT_FAILURE);
    }

    ssize_t bytes_received;
    size_t buffer_size = 1024; // initial buffer size
    char *buffer = malloc(buffer_size); // allocate initial buffer
    if (buffer == NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }

    size_t total_bytes_received = 0;

    while ((bytes_received = recv(client_socketfd, buffer + total_bytes_received, buffer_size - total_bytes_received, 0))) {
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
        // write only up to the newline character
        size_t bytes_to_write = newline - buffer + 1; // include the newline character
        fwrite(buffer, 1, bytes_to_write, data_file);
        syslog(LOG_INFO, "Received full data packet from %s", inet_ntoa(client_address.sin_addr));
        break;
      }
    }
    fclose(data_file);
    free(buffer);

    // return data to client
    data_file = fopen(DATA_FILE, "r");
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
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    fread(file_content, 1, file_size, data_file);
    fclose(data_file);

    // send to client
    send(client_socketfd, file_content, file_size, 0);

    // clean up
    free(file_content);
    close(client_socketfd);
    syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_address.sin_addr));
  }
  close(server_socketfd);
  return 0;
}
