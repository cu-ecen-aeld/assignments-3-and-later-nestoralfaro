CC ?= $(CROSS-COMPILE)gcc
TARGET ?= aesdsocket 
SRCS ?= $(wildcard *.c)
OBJS ?= $(SRCS:.c=.o)
CFLAGS ?= -Wall -Wextra -g
LDFLAGS ?= -lpthread -lrt

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(OBJS)
