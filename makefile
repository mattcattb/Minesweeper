CXX ?= c++
CPPFLAGS := -Isrc
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic

CORE_SOURCES := $(wildcard src/core/*.cpp)
CLIENT_SOURCES := $(wildcard src/client/*.cpp)
SERVER_SOURCES := $(wildcard src/server/*.cpp)
SFML_PREFIX = $(shell brew --prefix sfml@2 2>/dev/null)
SFML_PKG_CONFIG_PATH = $(if $(SFML_PREFIX),$(SFML_PREFIX)/lib/pkgconfig,)
SFML_CFLAGS = $(shell PKG_CONFIG_PATH="$(SFML_PKG_CONFIG_PATH):$$PKG_CONFIG_PATH" \
	pkg-config --cflags sfml-graphics sfml-window sfml-system)
SFML_LIBS = $(shell PKG_CONFIG_PATH="$(SFML_PKG_CONFIG_PATH):$$PKG_CONFIG_PATH" \
	pkg-config --libs sfml-graphics sfml-window sfml-system)

.PHONY: build client server test clean

build: client

client:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SFML_CFLAGS) \
		src/client_main.cpp $(CORE_SOURCES) $(CLIENT_SOURCES) \
		$(SFML_LIBS) -o minesweeper-client

server:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/server_main.cpp $(CORE_SOURCES) $(SERVER_SOURCES) \
		-pthread -o minesweeper-server

test:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/server_test.cpp $(CORE_SOURCES) $(SERVER_SOURCES) \
		-pthread -o minesweeper-server-test
	./minesweeper-server-test

clean:
	rm -f minesweeper-client minesweeper-server minesweeper-server-test
