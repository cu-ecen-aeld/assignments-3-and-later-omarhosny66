CC = $(CROSS_COMPILE)gcc
CFLAGS = -g -Wall -Werror -Wextra -O2 -MMD
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)
TARGET = aesdsocket

default: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

.PHONY: all clean default

-include $(DEPS)