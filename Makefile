# Predefined constants
CC      = cc
TARGET  = file-rom-bin
SRC_DIR = src
OBJ_DIR = obj

# GIMP 3.x. The 2.8/2.10 build used gtk+-2.0 + gimp-2.0 + gimpui-2.0;
# GIMP 3 plug-ins link against gimp-3.0 / gimpui-3.0 which pull in
# GTK3, GEGL and babl (needed for the GeglBuffer pixel API).
PKG_CONFIG   ?= pkg-config
GIMP_MODULES  = gimp-3.0 gimpui-3.0

# -Wno-sign-compare / -Wno-ignored-qualifiers quiet down long standing
# warnings in the format decoders (int vs unsigned loop counters) and one
# in a GIMP 3 header, so a fresh build stays readable.
CFLAGS  = -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
          -Wno-ignored-qualifiers -O2 -g \
          $(shell $(PKG_CONFIG) --cflags $(GIMP_MODULES))
LFLAGS  = $(shell $(PKG_CONFIG) --libs $(GIMP_MODULES))

# Where "make install" puts the plugin. GIMP also picks up
# ~/.config/GIMP/<version>/plug-ins and <prefix>/lib/gimp/<version>/plug-ins
GIMP_PLUGIN_DIR ?= $(HOME)/.config/GIMP/3.2/plug-ins

# File definitions
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

$(TARGET): $(OBJ_DIR) $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $(TARGET) $(LFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR):
	test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)

# GIMP 3 wants every plug-in binary in its own sub directory of plug-ins/
install: $(TARGET)
	mkdir -p $(GIMP_PLUGIN_DIR)/$(TARGET)
	cp $(TARGET) $(GIMP_PLUGIN_DIR)/$(TARGET)

uninstall:
	rm -rf $(GIMP_PLUGIN_DIR)/$(TARGET)

.PHONY: clean install uninstall
