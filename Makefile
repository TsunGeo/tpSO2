CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./src
TARGET = tp2_so

SOURCES = src/main.c src/disco/disco.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

clean:
	rm -f $(OBJECTS) $(TARGET) *.bin

.PHONY: all clean
