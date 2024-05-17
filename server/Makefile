CC ?= $(CROSS-COMPILE)gcc
TARGET ?= aesdsocket 
SRCS ?= $(wildcard *.c)
OBJS ?= $(SRCS:.c=.o)
CFLAGS ?= -Wall -Wextra -g
LIBS ?= -lpthread

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(OBJS)
