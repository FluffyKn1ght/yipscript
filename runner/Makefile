.DEFAULT_GOAL = all

LIBYS_VER_MAJOR := 0

TARGET := yiprun

CFLAGS ?=
LDFLAGS ?=

DEBUG ?= 1

ifeq ($(DEBUG),1)
	CFLAGS += -g -fsanitize=address -D _DEBUG
endif

SRC_DIR := src
LIBYS_BUILD_DIR := ../build
LIBYS_INCLUDE_DIR := ../include

LIBYIPSCRIPT = libyipscript.so.$(LIBYS_VER_MAJOR)

SOURCES := $(shell find $(SRC_DIR) -name "*.c")

$(TARGET): $(SOURCES)
	gcc $(CFLAGS) $(LDFLAGS) $^ -I$(LIBYS_INCLUDE_DIR) -L$(LIBYS_BUILD_DIR) -l:$(LIBYIPSCRIPT) -o $@

all: $(TARGET)

clean:
	rm -rf $(TARGET)

rebuild: clean
	$(MAKE)
