.DEFAULT_GOAL := all

VER_MAJOR := 0

TARGET = libyipscript.so.$(VER_MAJOR)

CC ?= gcc
LD ?= gcc

CFLAGS ?=
LDFLAGS ?=

DEBUG ?= 1

ifeq ($(DEBUG),1)
	CFLAGS += -g -fsanitize=address -D _DEBUG
endif

SRC_DIR := src
INCLUDE_DIR := include
BIN_DIR := bin
BUILD_DIR := build

SOURCES := $(shell find $(SRC_DIR) -name "*.c")
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SOURCES))

LIBRARIES :=
ifneq ($(LIBRARIES),)
	CFLAGS += $(shell pkgconf --cflags $(LIBRARIES))
	LDFLAGS += $(shell pkgconf --libs $(LIBRARIES))
endif

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) -c -fPIC -I$(INCLUDE_DIR) $(CFLAGS) $^ -o $@

$(TARGET): $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -shared $^ -o $(BUILD_DIR)/$@

all: $(TARGET)

clean:
	rm -rf $(BIN_DIR)
	rm -rf $(BUILD_DIR)

rebuild: clean
	make
