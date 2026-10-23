CXX = g++
CXXFLAGS ?= -O2
WARNINGS = -Wall -Wextra -Wno-unused-parameter
# Prefer pkg-config when installed; this machine has headers but no pkg-config.
PKG_CONFIG := $(shell command -v pkg-config 2>/dev/null)
ifneq ($(PKG_CONFIG),)
HYPR_CFLAGS := $(shell $(PKG_CONFIG) --cflags hyprland libei-1.0)
else
HYPR_CFLAGS := -I/usr/include/hyprland -I/usr/include/hyprland/src \
 -I/usr/include/hyprland/protocols -I/usr/include/libdrm \
 -I/usr/include/pixman-1 -I/usr/include/cairo -I/usr/include/libei-1.0
endif

.PHONY: all check clean
all: zoom-scroll.so

zoom-scroll.so: main.cpp zoom.hpp
	$(CXX) $(CXXFLAGS) $(WARNINGS) -std=c++23 -fno-gnu-unique -fPIC -shared $(HYPR_CFLAGS) main.cpp -o $@.tmp
	mv $@.tmp $@

test_zoom: test_zoom.cpp zoom.hpp
	$(CXX) $(CXXFLAGS) -Wall -Wextra -std=c++23 test_zoom.cpp -o $@

check: test_zoom
	./test_zoom

clean:
	rm -f zoom-scroll.so test_zoom
