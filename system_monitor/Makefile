CFLAGS ?= -g -Wall -Werror
TARGET ?= system_monitor_server
LDFLAGS ?= -lpthread
CC ?= $(CROSS_COMPILE)gcc
SRCS = system_monitor_server.c
OBJS = $(SRCS:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
