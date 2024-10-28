CC = gcc
CFLAGS = -I./include -Wall -g

TARGET = obj/buffertests

SRCS = tests/main-tests.c src/master-main.c data_structures/a_master_map.c \
       data_structures/dynamic_hash_map_string.c data_structures/lru_hash_map.c \
       data_structures/dynamic_hash_map_string_array.c

OBJS = $(SRCS:.c=.o)
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(TARGET)
.PHONY: all clean
