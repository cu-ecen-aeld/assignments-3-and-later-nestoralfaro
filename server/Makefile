# CC ?= $(CROSS-COMPILE)gcc
# TARGET ?= aesdsocket 
# SRCS ?= $(wildcard *.c)
# OBJS ?= $(SRCS:.c=.o)
# CFLAGS ?= -Wall -Wextra -g
# CFLAGS += -DUSE_AESD_CHAR_DEVICE=1
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

# ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
# 	CC ?= $(CROSS_COMPILE)gcc
# endif
#
# CFLAGS ?= -g -Wall -Werror
# LDFLAGS ?= -lpthread -lrt
# SRC := aesdsocket.c datafile.c
# TARGET ?= aesdsocket
# OBJS := $(SRC:.c=.o)
#
# EXTRA_CFLAGS = -DUSE_AESD_CHAR_DEVICE=$(if $(USE_AESD_CHAR_DEVICE),$(USE_AESD_CHAR_DEVICE),1)
#
# default_target: all
#
# # Targets
# all: $(TARGET)
#
# $(OBJS) : CFLAGS += $(EXTRA_CFLAGS)
# $(TARGET) : $(OBJS)	
# 	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)
#
# clean:
# 	$(RM) $(TARGET) $(OBJS)

.PHONY: all clean	
CC ?= gcc	
CROSS_COMPILE ?=

CFLAGS ?= -g -Wall -Werror 
SRCS = aesdsocket.c
TARGET ?= aesdsocket
LDFLAGS ?= -pthread -lrt

$(TARGET):$(TARGET).o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(LDFLAGS) -I/ $(TARGET).o -o $(TARGET)	

$(TARGET).o:$(TARGET).c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(LDFLAGS) -c $(TARGET).c -o $(TARGET).o	


all: $(TARGET)

clean:	
	rm -f *.o $(TARGET)
