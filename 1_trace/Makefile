# Trace
# Networks Program 1

PROGRAM=trace


OS = $(shell uname -s)
PROC = $(shell uname -p)
TARGET=$(PROGRAM)-$(OS)-$(PROC)

CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lpcap
SRC=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJECTS=$(patsubst %.c, %.o, $(SRC))


ifeq ("$(OS)", "SunOS")
	OSLIB=-L/opt/csw/lib -lsocket -lnsl
	OSINC=-I/opt/csw/include
	OSDEF=-DSOLARIS
else
ifeq ("$(OS)", "Darwin")
	OSLIB=
	OSINC=
	OSDEF=-DDARWIN
else
	OSLIB=
	OSINC=
	OSDEF=-DLINUX
endif
endif

all: $(TARGET)

clean:	
	rm -f $(OBJECTS) *~

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(TARGET): $(OBJECTS) $(HEADERS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) 