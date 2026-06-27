CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -I./src

TARGET = tp2_so

SOURCES = \
	src/main.c \
	src/disco/disco.c \
	src/i-node/inode.c \
	src/diretorio/diretorio.c \
	src/arquivo/arquivo.c \
	src/importacao/importacao.c

OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) *.bin

.PHONY: all clean