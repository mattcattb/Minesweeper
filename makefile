CXX ?= c++
CPPFLAGS := -Isrc
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic

BUILD_DIR := build
GAME_SOURCES := src/game.cpp
RUNTIME_SOURCES := \
	$(GAME_SOURCES) \
	src/game_state_file.cpp \
	src/runtime.cpp
SERVER_SOURCES := \
	$(RUNTIME_SOURCES) \
	src/leaderboard.cpp \
	src/protocol.cpp \
	src/tcp_server.cpp
UI_SOURCES := $(wildcard src/ui/*.cpp)
TEST_BINARIES := \
	$(BUILD_DIR)/tests/game_test \
	$(BUILD_DIR)/tests/game_state_file_test \
	$(BUILD_DIR)/tests/protocol_test \
	$(BUILD_DIR)/tests/runtime_test
JSON_CFLAGS = $(shell pkg-config --cflags nlohmann_json)
SFML_PREFIX = $(shell brew --prefix sfml@2 2>/dev/null)
SFML_PKG_CONFIG_PATH = $(if $(SFML_PREFIX),$(SFML_PREFIX)/lib/pkgconfig,)
SFML_CFLAGS = $(shell PKG_CONFIG_PATH="$(SFML_PKG_CONFIG_PATH):$$PKG_CONFIG_PATH" \
	pkg-config --cflags sfml-graphics sfml-window sfml-system)
SFML_LIBS = $(shell PKG_CONFIG_PATH="$(SFML_PKG_CONFIG_PATH):$$PKG_CONFIG_PATH" \
	pkg-config --libs sfml-graphics sfml-window sfml-system)

.PHONY: build client server test clean

build: server client

client: $(BUILD_DIR)/minesweeper-desktop

$(BUILD_DIR)/minesweeper-desktop: src/app/desktop_main.cpp $(GAME_SOURCES) $(UI_SOURCES)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SFML_CFLAGS) \
		$^ $(SFML_LIBS) -o $@

server: $(BUILD_DIR)/minesweeper-server

$(BUILD_DIR)/minesweeper-server: src/app/server_main.cpp $(SERVER_SOURCES)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(JSON_CFLAGS) $^ -pthread -o $@

test: $(TEST_BINARIES)
	@for test_binary in $(TEST_BINARIES); do \
		"$$test_binary" || exit 1; \
	done

$(BUILD_DIR)/tests/game_test: tests/game_test.cpp $(GAME_SOURCES)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/tests/protocol_test: tests/protocol_test.cpp src/protocol.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/tests/game_state_file_test: \
		tests/game_state_file_test.cpp $(RUNTIME_SOURCES)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(JSON_CFLAGS) $^ -o $@

$(BUILD_DIR)/tests/runtime_test: tests/runtime_test.cpp $(RUNTIME_SOURCES)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(JSON_CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)
