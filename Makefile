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
DEPS          := $(OBJS:.o=.d) $(TOBJS:.o=.d) $(OBJS:.o=.param.d) $(TOBJS:.o=.param.d)

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

$(RELEASE_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(PARAM_FLAGS) $(CFLAGS) -c $< -o $@
	$(PARAM_DEP_GEN) $< $@ $(PARAM_DIR) > $(RELEASE_BUILD_DIR)/$*.param.d

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(PARAM_FLAGS) $(CFLAGS) -c $< -o $@
	$(PARAM_DEP_GEN) $< $@ $(PARAM_DIR) > $(TEST_BUILD_DIR)/$*.param.d

$(BUILD_DIR) $(RELEASE_BUILD_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

.PHONY: all clean run test

clean:
	rm -rf $(BUILD_DIR) $(RELEASE_BUILD_DIR) $(TEST_BUILD_DIR)

run: $(TARGET_RELEASE)
	./$(TARGET_RELEASE)

test: $(TARGET_TEST)
	./$(TARGET_TEST)

-include $(DEPS)
