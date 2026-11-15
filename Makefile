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
TARGET_PARAMS     := $(BUILD_DIR)/params.env

SRCS   := $(wildcard $(SRC_DIR)/*.c)
OBJS   := $(patsubst $(SRC_DIR)/%.c,$(RELEASE_BUILD_DIR)/%.o,$(SRCS))
TESTS  := $(wildcard $(TEST_DIR)/*.c)
TOBJS  := $(patsubst $(TEST_DIR)/%.c,$(TEST_BUILD_DIR)/%.o,$(TESTS))
PARAMS := $(wildcard $(PARAM_DIR)/*.param)
DEPS   := $(OBJS:.o=.d) $(TOBJS:.o=.d) $(OBJS:.o=.params.d) $(TOBJS:.o=.params.d)

CFLAGS  := -std=c23 -Wall -Wextra -Wpedantic -Wshadow -Werror -Iinclude --embed-dir=. -MMD -MP
CFLAGS  += -O3 -flto=auto
LDFLAGS := -static -flto=auto

LD := $(CC)
all: $(TARGET_RELEASE) $(TARGET_TEST)

$(TARGET_RELEASE): $(OBJS) | $(RELEASE_BUILD_DIR)
	$(LD) $(LDFLAGS) $^ -o $@

$(TARGET_TEST): $(filter-out $(RELEASE_BUILD_DIR)/nyx.o,$(OBJS)) $(TOBJS) | $(TEST_BUILD_DIR)
	$(LD) $(LDFLAGS) $^ -o $@

$(RELEASE_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_BUILD_DIR) $(TARGET_PARAMS)
	$(CC) $(CPPFLAGS) $$(awk '{printf " -D%s", $$0}' $(TARGET_PARAMS)) $(CFLAGS) -c $< -o $@
	tools/paramdep_generator.sh $< $@ $(PARAM_DIR) > $(@:.o=.params.d)

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR) $(TARGET_PARAMS)
	$(CC) $(CPPFLAGS) $$(awk '{printf " -D%s", $$0}' $(TARGET_PARAMS)) $(CFLAGS) -c $< -o $@
	tools/paramdep_generator.sh $< $@ $(PARAM_DIR) > $(@:.o=.params.d)

$(TARGET_PARAMS): $(PARAMS) | $(BUILD_DIR)
	for f in $(PARAMS); do name="$${f##*/}"; name="$${name%.param}"; printf "NYX_PARAMETER_%s=%s\n" "$$name" "$$(head -n 1 "$$f")"; done > $(TARGET_PARAMS)

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
