CFLAGS ?= -g -Wall -Werror
TARGET ?= system_data_sender
LDFLAGS ?= -lpthread
CC ?= $(CROSS_COMPILE)gcc
SRCS = system_data_sender.c
OBJS = $(SRCS:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
