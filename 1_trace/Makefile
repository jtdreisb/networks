# Trace
# Networks Program 1

TARGET=trace

CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lm
SRC=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJECTS=$(patsubst %.c, %.o, $(SRC))

all: $(TARGET)

clean:	
	rm -f $(OBJECTS) *~

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(TARGET): $(OBJECTS) $(HEADERS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) 