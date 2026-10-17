CXX ?= clang++
CXXFLAGS ?= -O3 -Wall -Wextra -Wpedantic -Werror
CXXFLAGS += -std=c++23

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

.PHONY: all clean install

all: fclang fclang++

fclang: fclang.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

fclang++: fclang.cpp
	$(CXX) $(CXXFLAGS) -DFCLANG_FORCE_CPP $< -o $@

clean:
	rm -f fclang fclang++

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 fclang $(DESTDIR)$(BINDIR)/fclang
	install -m 755 fclang++ $(DESTDIR)$(BINDIR)/fclang++

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/fclang $(DESTDIR)$(BINDIR)/fclang++