SRC_DIR        := nyx
BUILD_DIR      := build
LUT_DIR        := $(BUILD_DIR)/lut
TARGET_RELEASE := $(BUILD_DIR)/nyx
TARGET_LUT_GEN := $(BUILD_DIR)/nyx-lutgen

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

LUTS += bishop_mask
LUTS += bishop_offset
LUTS += bishop_attacks
LUTS += rook_mask
LUTS += rook_offset
LUTS += rook_attacks
LUTS += knight_attacks
LUTS += king_attacks
LUTS := $(addprefix $(LUT_DIR)/,$(addsuffix .bin,$(LUTS)))

CFLAGS := -Wall -Wextra -Wpedantic -Werror -I. -MMD -MP
CFLAGS += -march=native

all: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LUT_GEN): tools/attacks_lut_gen.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/movegen.o: $(LUTS)
$(LUTS): $(TARGET_LUT_GEN) | $(LUT_DIR)
	./$(TARGET_LUT_GEN) $(LUT_DIR)

$(BUILD_DIR) $(LUT_DIR):
	mkdir -p $@

.PHONY: all clean run

clean:
	rm -rf $(BUILD_DIR) $(LUT_DIR)

run: $(TARGET_RELEASE)
	./$(TARGET_RELEASE)

-include $(DEPS)
