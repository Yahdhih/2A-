# Makefile for MLIFE project with two executables

# Compiler settings
CC = mpicc
CFLAGS = -Wall -O2

# Source files
SRCS = mlife.c mlife-io-stdout.c mlife-fence.c mlife-pscw.c mlife-pt2pt.c
OBJS = $(SRCS:.c=.o)

# Header files
HDRS = mlife.h mlife-io.h

# Executables
TARGET_PT2PT = mlife-pt2pt
TARGET_FENCE = mlife-fence
TARGET_PSCW = mlife-pscw

# Build rules
all: $(TARGET_PT2PT) $(TARGET_FENCE) $(TARGET_PSCW)

$(TARGET_PT2PT): $(filter-out mlife-fence.o mlife-pscw.o, $(OBJS))
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_FENCE): $(filter-out mlife-pt2pt.o mlife-pscw.o, $(OBJS))
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_PSCW): $(filter-out mlife-fence.o mlife-pt2pt.o, $(OBJS))
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET_PT2PT) $(TARGET_FENCE) $(TARGET_PSCW)

