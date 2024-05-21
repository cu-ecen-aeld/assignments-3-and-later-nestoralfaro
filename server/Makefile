# CC ?= $(CROSS-COMPILE)gcc
# TARGET ?= aesdsocket 
# SRCS ?= $(wildcard *.c)
# OBJS ?= $(SRCS:.c=.o)
# CFLAGS ?= -Wall -Wextra -g
# LDFLAGS ?= -lpthread -lrt
#
# .PHONY: all clean
#
# all: $(TARGET)
#
# $(TARGET): $(OBJS)
# 	$(CC) $^ -o $@ $(LDFLAGS)
#
# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@
#
# clean:
# 	$(RM) $(TARGET) $(OBJS)



# # sort of working
# CC ?= $(CROSS_COMPILE)gcc
# CFLAGS ?= -g -Wall -Werror
# LDFLAGS ?= -pthread -lrt
# TARGET ?= aesdsocket
#
# all: $(TARGET)
#
# $(TARGET): aesdsocket.o
# 	@$(CC) aesdsocket.o -o $(TARGET) $(LDFLAGS)
#
# %.o: %.c
# 	@$(CC) $(CFLAGS) -c $< -o $@
#
# .PHONY: clean
# clean:
# 	@rm -f *.o $(TARGET)






CC ?= $(CROSS_COMPILE) gcc
CFLAGS ?= -g -pthread
LDFLAGS ?= -lm

# ifeq ($(CROSS_COMPILE),)
# 	$(info Compiling natively.)
# else
# 	CROSS_COMPILE_FULL = aarch64-none-linux-gnu-$(CROSS_COMPILE)
# 	CC = $(CROSS_COMPILE_FULL)
# 	$(info Cross compiling: $(CROSS_COMPILE_FULL))
# endif

all: clean aesdsocket.o aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket $(LDFLAGS)

aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c -o aesdsocket.o $(LDFLAGS)

clean:
	rm -f aesdsocket aesdsocket.o
