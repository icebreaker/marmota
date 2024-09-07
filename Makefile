CP ?= cp
RM ?= rm -f
SED ?= sed -i

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
APP_DIR = $(PREFIX)/share/applications

BUILD_DIR=build

BUILD_TARGET=$(BUILD_DIR)/$(TARGET)
BUILD_CONFIG=$(BUILD_DIR)/config.h
BUILD_DESKTOP=$(BUILD_DIR)/$(TARGET).desktop

INSTALL_DESKTOP_TARGET=$(APP_DIR)/$(TARGET).desktop
INSTALL_TARGET=$(BIN_DIR)/$(TARGET)

SOURCES = src/main.c src/marmota.c src/marmota.h

all: $(BUILD_TARGET)

config: $(BUILD_CONFIG)

$(BUILD_CONFIG):
	$(CP) src/config.h.in $(BUILD_CONFIG)

$(BUILD_DESKTOP):
	$(CP) res/marmota.desktop.in $(BUILD_DESKTOP)

$(BUILD_TARGET): $(SOURCES) $(BUILD_CONFIG) $(BUILD_DESKTOP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) `pkg-config --cflags --libs gtk+-3.0 vte-2.91`

install: $(BUILD_TARGET)
	$(INSTALL) -D $(BUILD_TARGET) $(INSTALL_TARGET)

install-desktop: $(BUILD_DESKTOP)
	$(SED) "s|^Exec=.*|Exec=$(INSTALL_TARGET)|g" $(BUILD_DESKTOP)
	$(INSTALL) -D $(BUILD_DESKTOP) $(INSTALL_DESKTOP_TARGET)

clean:
	$(RM) $(BUILD_TARGET)

distclean: clean
	$(RM) $(BUILD_CONFIG) $(BUILD_DESKTOP)

.PHONY: all config install installdirs clean distclean
