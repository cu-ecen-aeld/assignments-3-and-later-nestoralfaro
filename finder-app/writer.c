#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
	openlog("writer", LOG_PID | LOG_CONS, LOG_USER | LOG_DEBUG);

	if (argc != 3) {
		syslog(LOG_ERR, "Usage: %s <pathname> <content>", argv[0]);
		closelog();
		return EXIT_FAILURE;
	}

	const char *pathname = argv[1];
	const char *content = argv[2];

	// As seen on "Permissions of New Files" from FILE I/O Chapter 2 LSP.
	int fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		syslog(LOG_ERR, "Failed to open file %s for writing.", pathname);
		closelog();
		return EXIT_FAILURE;
	}

	syslog(LOG_DEBUG, "Writing %s to %s", content, pathname);

	ssize_t bytes_written = write(fd, content, strlen(content));
	if (bytes_written == -1) {
		syslog(LOG_ERR, "Error writing to file %s", pathname);
		closeSafely(fd);
		closelog();
		return EXIT_FAILURE;
	}

	closeSafely(fd);
	closelog();
	return EXIT_SUCCESS;
}

void closeSafely(int fd) {
	if (close(fd) == -1) {
		syslog(LOG_ERR, "Error closing file. Attempting to sync.");
		if (fdatasync(fd) == -1) {
			syslog(LOG_ERR, "Failed to synchronized file descriptor. Now synchronizing all buffers to disk");
			sync();
		}
	}
}
