CFLAGS += -std=c99 -Wall -Wextra -Werror -O3 -Isrc -Ibuild

ifdef MAX_COLORS
CFLAGS += -DMRT_MAX_COLORS=$(MAX_COLORS)
endif

ifdef FALLBACK_SHELL
CFLAGS += -DMRT_FALLBACK_SHELL=$(FALLBACK_SHELL)
endif

TARGET ?= marmota
INSTALL ?= install
PREFIX ?= /usr/local

BIN_DIR = $(PREFIX)/bin
BUILD_DIR=build
BUILD_TARGET=$(BUILD_DIR)/$(TARGET)
BUILD_CONFIG=$(BUILD_DIR)/config.h

SOURCES = src/main.c src/marmota.c src/marmota.h

all: $(BUILD_TARGET)

config: $(BUILD_CONFIG)

$(BUILD_CONFIG):
	cp src/config.h.in $(BUILD_CONFIG)

$(BUILD_TARGET): $(SOURCES) $(BUILD_CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) `pkg-config --cflags --libs gtk+-3.0 vte-2.91`

install: $(BUILD_TARGET)
	$(INSTALL) -D $(BUILD_TARGET) $(BIN_DIR)/$(TARGET)

clean:
	rm -f $(BUILD_TARGET)

distclean: clean
	rm -f $(BUILD_CONFIG)

.PHONY: all config install installdirs clean distclean
