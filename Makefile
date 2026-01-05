# Makefile for Zircon (C implementation)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lgit2 -lcurl -larchive -lz -lcrypto

# Source files
SRCS = src/main.c \
       src/common.c \
       src/cli.c \
       src/paths.c \
       src/config.c \
       src/git_utils.c \
       src/deps.c \
       src/build.c \
       src/toolchains.c \
       src/update_check.c \
       src/cmds/build_cmds.c \
       src/cmds/env_cmds.c \
       src/cmds/install_cmds.c \
       src/cmds/internal_cmds.c \
       src/cmds/self_cmds.c \
       src/cmds/toolchain_cmds.c

# Object files
OBJS = $(SRCS:.c=.o)

# Output binary
TARGET = zircon

# Default target
all: $(TARGET)

# Link the target
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Install target
install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

# Phony targets
.PHONY: all clean install
