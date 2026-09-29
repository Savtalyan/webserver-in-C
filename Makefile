CC      := gcc
CFLAGS  := -std=gnu11 -Wall -Wextra -Wpedantic -O2
LDFLAGS :=

SRC_DIR   := src
BUILD_DIR := build
TARGET    := server

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all debug run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# -MMD -MP: generate .d files so editing a header rebuilds the .c files that include it
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

# clean rebuild with debug info + AddressSanitizer/UBSan
debug: clean
	$(MAKE) CFLAGS="-std=gnu11 -Wall -Wextra -Wpedantic -g -O0 -fsanitize=address,undefined" LDFLAGS="-fsanitize=address,undefined" all

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)