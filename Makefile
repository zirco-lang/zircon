V ?= 0
DEBUG ?= 0

ifeq ($(V),1)
Q :=
ECHO := @true
else
Q := @
ECHO := @echo
MAKEFLAGS += --no-print-directory
endif

SRCDIR := src
OUTDIR := dist

CC := clang
CFLAGS ?= -Wall -Wextra -Wpedantic -Wconversion -std=c11 -D_POSIX_C_SOURCE=200809L
LDLIBS ?= -larchive -lcurl -lssl -lcrypto
LDFLAGS ?= 

ifeq ($(DEBUG),1)
CFLAGS += -O0 -g3 -DDEBUG
else
CFLAGS += -O2 -DNDEBUG -flto=thin
LDFLAGS += -flto=thin
endif

ifeq ($(shell uname),Darwin)
LDFLAGS += -mmacosx-version-min=26.0

# Homebrew libarchive and libssl is not in the default search path, so we need to add it
LDFLAGS += -L/opt/homebrew/opt/libarchive/lib -L/opt/homebrew/opt/openssl@3/lib
LDFLAGS += -L/usr/local/opt/libarchive/lib -L/usr/local/opt/openssl@3/lib
CFLAGS += -I/opt/homebrew/opt/libarchive/include -I/opt/homebrew/opt/openssl@3/include
CFLAGS += -I/usr/local/opt/libarchive/include -I/usr/local/opt/openssl@3/include
endif

C_SOURCES := $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/cmds/*.c)
C_OUTPUTS := $(C_SOURCES:$(SRCDIR)/%.c=$(OUTDIR)/%.o)
TARGET := zircon

.PHONY: all
all: $(OUTDIR)/$(TARGET)

$(OUTDIR)/$(TARGET): $(C_OUTPUTS) | $(OUTDIR)
	$(ECHO) "  CCLD   $@"
	$(Q)$(CC) -o $(OUTDIR)/$(TARGET) $(C_OUTPUTS) $(LDLIBS) $(LDFLAGS)

$(OUTDIR)/%.o: $(SRCDIR)/%.c | $(OUTDIR)
	$(ECHO) "  CC     $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(OUTDIR):
	$(ECHO) "  MKDIR  $@"
	$(Q)mkdir -p $(OUTDIR)/cmds

.PHONY: clean
clean:
	$(ECHO) "  RM     $(OUTDIR)"
	$(Q)rm -rf $(OUTDIR)
