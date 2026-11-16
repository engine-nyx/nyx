# Copyright (c) 2026 Kilian Chung
# SPDX-License-Identifier: NLPL

SRC_DIR           := src
TEST_DIR          := test
PARAM_DIR         := params
BUILD_DIR         := build
RELEASE_BUILD_DIR := $(BUILD_DIR)/release
TEST_BUILD_DIR    := $(BUILD_DIR)/test
TARGET_RELEASE    := $(RELEASE_BUILD_DIR)/nyx
TARGET_TEST       := $(TEST_BUILD_DIR)/test

SRCS          := $(wildcard $(SRC_DIR)/*.c)
OBJS          := $(patsubst $(SRC_DIR)/%.c,$(RELEASE_BUILD_DIR)/%.o,$(SRCS))
TESTS         := $(wildcard $(TEST_DIR)/*.c)
TOBJS         := $(patsubst $(TEST_DIR)/%.c,$(TEST_BUILD_DIR)/%.o,$(TESTS))
PARAMS        := $(wildcard $(PARAM_DIR)/*.param)
PARAM_DEP_GEN := tools/paramdep_generator.sh
DEPS          := $(OBJS:.o=.d) $(TOBJS:.o=.d) $(OBJS:.o=.params.d) $(TOBJS:.o=.params.d)

# The first line of each params/*.param file is its value.
# $(file) needs GNU make 4.2+ but avoids awk/head and the generated params.env.
param_value = $(firstword $(file <$(1)))
PARAM_FLAGS := $(foreach f,$(PARAMS),-DNYX_PARAMETER_$(basename $(notdir $(f)))=$(call param_value,$(f)))

CFLAGS  := -std=c23 -Wall -Wextra -Wpedantic -Wshadow -Werror -Iinclude --embed-dir=. -MMD -MP
CFLAGS  += -O3 -flto=auto
LDFLAGS := -static -flto=auto

LD := $(CC)
all: $(TARGET_RELEASE) $(TARGET_TEST)

$(TARGET_RELEASE): $(OBJS) | $(RELEASE_BUILD_DIR)
	$(LD) $(LDFLAGS) $^ -o $@

$(TARGET_TEST): $(filter-out $(RELEASE_BUILD_DIR)/nyx.o,$(OBJS)) $(TOBJS) | $(TEST_BUILD_DIR)
	$(LD) $(LDFLAGS) $^ -o $@

# The scanner emits only the params actually used by each source, so a change
# to a param rebuilds just the objects that reference it. Keeping the dep files
# as targets also recreates them if they are deleted.
$(RELEASE_BUILD_DIR)/%.params.d: $(SRC_DIR)/%.c $(PARAM_DEP_GEN) | $(RELEASE_BUILD_DIR)
	$(PARAM_DEP_GEN) $< $(RELEASE_BUILD_DIR)/$*.o $(PARAM_DIR) > $@

$(TEST_BUILD_DIR)/%.params.d: $(TEST_DIR)/%.c $(PARAM_DEP_GEN) | $(TEST_BUILD_DIR)
	$(PARAM_DEP_GEN) $< $(TEST_BUILD_DIR)/$*.o $(PARAM_DIR) > $@

$(RELEASE_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(PARAM_FLAGS) $(CFLAGS) -c $< -o $@

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(PARAM_FLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR) $(RELEASE_BUILD_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

.PHONY: all clean run test

clean:
	rm -rf $(BUILD_DIR) $(RELEASE_BUILD_DIR) $(TEST_BUILD_DIR)

run: $(TARGET_RELEASE)
	./$(TARGET_RELEASE)

test: $(TARGET_TEST)
	./$(TARGET_TEST)

ifneq ($(filter clean,$(MAKECMDGOALS)),)
# `make clean` should not regenerate dependency files just to delete them.
else
-include $(DEPS)
endif
