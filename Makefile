CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Wno-unused-parameter
INCLUDES = -Isrc/core -Isrc/frontend -Isrc/runtime -Isrc/stdlib -Isrc/pm -Isrc/test_runner -Isrc/formatter
LDFLAGS = -lm
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
endif

BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj

SRCS = src/core/value.c src/core/env.c \
       src/frontend/lexer.c src/frontend/parser.c \
       src/runtime/interpreter.c src/runtime/repl.c \
       src/stdlib/net.c src/stdlib/os_module.c \
       src/stdlib/time_module.c src/stdlib/path_module.c \
       src/stdlib/http_module.c \
       src/stdlib/sqlite3.c src/stdlib/sqlite_module.c \
       src/stdlib/cli_module.c \
       src/stdlib/regex_engine.c src/stdlib/regex_module.c \
       src/stdlib/crypto_module.c src/stdlib/uuid_module.c \
       src/stdlib/url_module.c src/stdlib/log_module.c \
       src/stdlib/env_module.c \
       src/test_runner/test_runner.c \
       src/formatter/formatter.c \
       src/pm/pm.c \
       src/main.c

OBJS = $(OBJDIR)/value.o $(OBJDIR)/env.o \
       $(OBJDIR)/lexer.o $(OBJDIR)/parser.o \
       $(OBJDIR)/interpreter.o $(OBJDIR)/repl.o \
       $(OBJDIR)/net.o $(OBJDIR)/os_module.o \
       $(OBJDIR)/time_module.o $(OBJDIR)/path_module.o \
       $(OBJDIR)/http_module.o \
       $(OBJDIR)/sqlite3.o $(OBJDIR)/sqlite_module.o \
       $(OBJDIR)/cli_module.o \
       $(OBJDIR)/regex_engine.o $(OBJDIR)/regex_module.o \
       $(OBJDIR)/crypto_module.o $(OBJDIR)/uuid_module.o \
       $(OBJDIR)/url_module.o $(OBJDIR)/log_module.o \
       $(OBJDIR)/env_module.o \
       $(OBJDIR)/test_runner.o \
       $(OBJDIR)/formatter.o \
       $(OBJDIR)/pm.o \
       $(OBJDIR)/main.o

ifeq ($(OS),Windows_NT)
    TARGET = $(BUILDDIR)/melody.exe
    RM = del /Q 2>NUL
    RMDIR = rmdir /S /Q 2>NUL
    MKDIR = if not exist $(subst /,\,$1) mkdir $(subst /,\,$1)
else
    TARGET = $(BUILDDIR)/melody
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p $1
endif

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Core
$(OBJDIR)/value.o: src/core/value.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/env.o: src/core/env.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Frontend
$(OBJDIR)/lexer.o: src/frontend/lexer.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/parser.o: src/frontend/parser.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Runtime
$(OBJDIR)/interpreter.o: src/runtime/interpreter.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/repl.o: src/runtime/repl.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Standard Library
$(OBJDIR)/net.o: src/stdlib/net.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/os_module.o: src/stdlib/os_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/time_module.o: src/stdlib/time_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/path_module.o: src/stdlib/path_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/http_module.o: src/stdlib/http_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/sqlite3.o: src/stdlib/sqlite3.c | $(OBJDIR)
	$(CC) -std=c11 -O2 -w -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -c -o $@ $<

$(OBJDIR)/sqlite_module.o: src/stdlib/sqlite_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/cli_module.o: src/stdlib/cli_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# New stdlib modules
$(OBJDIR)/regex_engine.o: src/stdlib/regex_engine.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/regex_module.o: src/stdlib/regex_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/crypto_module.o: src/stdlib/crypto_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/uuid_module.o: src/stdlib/uuid_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/url_module.o: src/stdlib/url_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/log_module.o: src/stdlib/log_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJDIR)/env_module.o: src/stdlib/env_module.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Test runner
$(OBJDIR)/test_runner.o: src/test_runner/test_runner.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Formatter
$(OBJDIR)/formatter.o: src/formatter/formatter.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Package manager
$(OBJDIR)/pm.o: src/pm/pm.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Entry point
$(OBJDIR)/main.o: src/main.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILDDIR):
	$(call MKDIR,$(BUILDDIR))

$(OBJDIR):
	$(call MKDIR,$(OBJDIR))

# Header dependencies
$(OBJDIR)/value.o: src/core/value.h src/core/env.h
$(OBJDIR)/env.o: src/core/env.h src/core/value.h
$(OBJDIR)/lexer.o: src/frontend/lexer.h src/core/value.h
$(OBJDIR)/parser.o: src/frontend/parser.h src/frontend/lexer.h src/core/value.h
$(OBJDIR)/interpreter.o: src/runtime/interpreter.h src/frontend/parser.h src/core/env.h src/core/value.h
$(OBJDIR)/repl.o: src/runtime/repl.h src/runtime/interpreter.h
$(OBJDIR)/net.o: src/stdlib/net.h src/core/value.h
$(OBJDIR)/os_module.o: src/stdlib/os_module.h src/core/value.h
$(OBJDIR)/time_module.o: src/stdlib/time_module.h src/core/value.h
$(OBJDIR)/path_module.o: src/stdlib/path_module.h src/core/value.h
$(OBJDIR)/http_module.o: src/stdlib/http_module.h src/core/value.h
$(OBJDIR)/sqlite_module.o: src/stdlib/sqlite_module.h src/stdlib/sqlite3.h src/core/value.h
$(OBJDIR)/cli_module.o: src/stdlib/cli_module.h src/core/value.h
$(OBJDIR)/regex_engine.o: src/stdlib/regex_engine.h
$(OBJDIR)/regex_module.o: src/stdlib/regex_module.h src/stdlib/regex_engine.h src/core/value.h
$(OBJDIR)/crypto_module.o: src/stdlib/crypto_module.h src/core/value.h
$(OBJDIR)/uuid_module.o: src/stdlib/uuid_module.h src/core/value.h
$(OBJDIR)/url_module.o: src/stdlib/url_module.h src/core/value.h
$(OBJDIR)/log_module.o: src/stdlib/log_module.h src/core/value.h
$(OBJDIR)/env_module.o: src/stdlib/env_module.h src/core/value.h
$(OBJDIR)/test_runner.o: src/test_runner/test_runner.h src/runtime/interpreter.h src/core/value.h
$(OBJDIR)/formatter.o: src/formatter/formatter.h src/frontend/lexer.h
$(OBJDIR)/pm.o: src/pm/pm.h
$(OBJDIR)/main.o: src/runtime/interpreter.h src/runtime/repl.h src/pm/pm.h src/test_runner/test_runner.h src/formatter/formatter.h

clean:
	$(RMDIR) $(BUILDDIR)

debug: CFLAGS += -g -O0 -DDEBUG
debug: clean all

.PHONY: all clean debug
