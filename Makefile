# ---- Project ----
TARGET   := server

SRCDIR   := src
INCDIR   := includes
BUILDDIR := build

SRCS     := $(addprefix $(SRCDIR)/, main.c buffer.c http.c ring_buffer.c socket.c thread.c utils.c)
OBJS     := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
DEPS     := $(OBJS:.o=.d)

# ---- Toolchain / flags ----
CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -I$(INCDIR) -D_POSIX_C_SOURCE=200809L -MMD -MP
LDFLAGS  ?=
LDLIBS   += -pthread

# `make DEBUG=1` for a debug build with sanitizers
ifeq ($(DEBUG),1)
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O0 -g3 -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif

# ---- Rules ----
.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -pthread -c $< -o $@

$(BUILDDIR):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

# Auto-generated header dependencies (rebuilds a .o when any .h it includes changes)
-include $(DEPS)