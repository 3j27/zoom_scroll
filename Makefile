CXX = g++
CXXFLAGS ?= -O2
WARNINGS = -Wall -Wextra -Wno-unused-parameter
PKG_CONFIG ?= pkg-config
HYPR_CFLAGS = $(shell $(PKG_CONFIG) --cflags hyprland libei-1.0)

.PHONY: all check clean
all: zoom-scroll.so

zoom-scroll.so: main.cpp zoom.hpp
	@$(PKG_CONFIG) --exists hyprland libei-1.0 || { echo "Install pkgconf and matching Hyprland/libei development files first." >&2; exit 1; }
	$(CXX) $(CXXFLAGS) $(WARNINGS) -std=c++23 -fno-gnu-unique -fPIC -shared $(HYPR_CFLAGS) main.cpp -o $@.tmp
	mv $@.tmp $@

test_zoom: test_zoom.cpp zoom.hpp
	$(CXX) $(CXXFLAGS) -Wall -Wextra -std=c++23 test_zoom.cpp -o $@

check: test_zoom
	./test_zoom

clean:
	rm -f zoom-scroll.so zoom-scroll.so.tmp test_zoom
