.DEFAULT_GOAL := run
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iincludefile
SRC_DIR = src
BUILD_DIR = build
TARGET = mypocket
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

ifeq ($(OS),Windows_NT)
EXE = $(TARGET).exe
RUN = .\$(EXE)
MKDIR_BUILD = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
CLEAN_CMD = if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR) & if exist $(TARGET).exe del /Q $(TARGET).exe & if exist $(TARGET) del /Q $(TARGET)
else
EXE = $(TARGET)
RUN = ./$(EXE)
MKDIR_BUILD = mkdir -p $(BUILD_DIR)
CLEAN_CMD = rm -rf $(BUILD_DIR) $(TARGET) $(TARGET).exe
endif

.PHONY: run build clean help

# Default: build and launch the interactive console app.
run: $(EXE)
	$(RUN)

# Compile only; useful for checking the project without opening the menu.
build: $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c includefile/budget.h
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	@$(CLEAN_CMD)

help:
	@echo "make       - compile and run MyPocket"
	@echo "make build - compile only"
	@echo "make clean - remove build files"

# Core regression tests run in an isolated sandbox so your real data/ is untouched.
TEST_TARGET = $(BUILD_DIR)/test_core$(if $(filter Windows_NT,$(OS)),.exe,)
TEST_SOURCES = $(filter-out $(SRC_DIR)/main.c,$(SOURCES))

.PHONY: test

test: $(TEST_TARGET)
ifeq ($(OS),Windows_NT)
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@if not exist $(BUILD_DIR)\\test_sandbox mkdir $(BUILD_DIR)\\test_sandbox
	@if not exist $(BUILD_DIR)\\test_sandbox\\data mkdir $(BUILD_DIR)\\test_sandbox\\data
	@cd /d $(BUILD_DIR)\\test_sandbox && ..\\test_core.exe
else
	@mkdir -p $(BUILD_DIR)/test_sandbox/data
	@cd $(BUILD_DIR)/test_sandbox && ../test_core
endif

$(TEST_TARGET): tests/test_core.c $(TEST_SOURCES) includefile/budget.h
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_core.c $(TEST_SOURCES) -o $@
