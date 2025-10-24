# Makefile for MLIFE project with two executables

# Compiler settings
CC = mpicc
CFLAGS = -Wall -O2

# Source files
SRCS = mlife.c mlife-io-mpiio.c mlife-io-stdout.c mlife-pt2pt.c
OBJS = $(SRCS:.c=.o)

# Header files
HDRS = mlife.h mlife-io.h

# Executables
TARGET = mlife
TARGET_MPIIO = mlife-mpiio

# Build rules
all: $(TARGET) $(TARGET_MPIIO)

$(TARGET): $(filter-out mlife-io-mpiio.o, $(OBJS))
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_MPIIO): $(filter-out mlife-io-stdout.o, $(OBJS))
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET_MPIIO)
