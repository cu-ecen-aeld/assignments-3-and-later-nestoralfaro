CC ?= $(CROSS-COMPILE)gcc
TARGET ?= aesdsocket 
SRCS ?= $(wildcard *.c)
OBJS ?= $(SRCS:.c=.o)
CFLAGS ?= -Wall -Wextra -g
CFLAGS += -DUSE_AESD_CHAR_DEVICE=1
LDFLAGS ?= -lpthread -lrt

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(OBJS)
