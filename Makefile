SRC_DIR        := nyx
BUILD_DIR      := build
TARGET_RELEASE := $(BUILD_DIR)/nyx

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

CFLAGS := -Wall -Wextra -Wpedantic -Werror -I. -MMD -MP

all: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

.PHONY: all clean run

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET_RELEASE)
	./$(TARGET_RELEASE)

-include $(DEPS)
