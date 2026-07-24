SRC_DIR           := src
TEST_DIR          := test
BUILD_DIR         := build
RELEASE_BUILD_DIR := $(BUILD_DIR)/release
TEST_BUILD_DIR    := $(BUILD_DIR)/test
LUT_DIR           := $(BUILD_DIR)/lut
TARGET_RELEASE    := $(RELEASE_BUILD_DIR)/nyx
TARGET_TEST       := $(TEST_BUILD_DIR)/test
TARGET_LUT_GEN    := $(LUT_DIR)/nyx-lutgen

SRCS  := $(wildcard $(SRC_DIR)/*.c)
OBJS  := $(patsubst $(SRC_DIR)/%.c,$(RELEASE_BUILD_DIR)/%.o,$(SRCS))
TESTS := $(wildcard $(TEST_DIR)/*.c)
TOBJS := $(patsubst $(TEST_DIR)/%.c,$(TEST_BUILD_DIR)/%.o,$(TESTS))
DEPS  := $(OBJS:.o=.d) $(TOBJS:.o=.d)

LUTS += bishop_mask
LUTS += bishop_offset
LUTS += bishop_attacks
LUTS += rook_mask
LUTS += rook_offset
LUTS += rook_attacks
LUTS += knight_attacks
LUTS += king_attacks
LUTS := $(addprefix $(LUT_DIR)/,$(addsuffix .bin,$(LUTS)))

CFLAGS := -Wall -Wextra -Wpedantic -Werror -Iinclude -MMD -MP
CFLAGS += -march=native
CLFAGS += std=c23

all: $(TARGET_RELEASE) $(TARGET_TEST)

$(TARGET_RELEASE): $(OBJS) | $(RELEASE_BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_TEST): $(filter-out $(RELEASE_BUILD_DIR)/nyx.o,$(OBJS)) $(TOBJS) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(RELEASE_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LUT_GEN): tools/attacks_lut_gen.c | $(LUT_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(LUTS): $(TARGET_LUT_GEN) | $(LUT_DIR)
	./$(TARGET_LUT_GEN) $(LUT_DIR)

$(BUILD_DIR) $(RELEASE_BUILD_DIR) $(TEST_BUILD_DIR) $(LUT_DIR):
	mkdir -p $@

.PHONY: all clean run

clean:
	rm -rf $(BUILD_DIR) $(RELEASE_BUILD_DIR) $(TEST_BUILD_DIR) $(LUT_DIR)

run: $(TARGET_RELEASE)
	./$(TARGET_RELEASE)

test: $(TARGET_TEST)
	./$(TARGET_TEST)

-include $(DEPS)
