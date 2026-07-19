# Minesweeper - Fall 2023 Project

UPDATED for a event loop + websocket server to be ran as a docker container!!!

See break my mindsweeper for a example implemnetaion of this! Includes a event loop with task management, saving of queues nad data nd more here througout the applcaiton :33333 ARGGGG RAAAAA JFEEJIOFEJFOEJ KILLLLL MEEEE

##### **Name**: Matthew Boughton

##### **System**: Ubuntu 22.04.2 LTS

##### **Compiler**: 11.4.0

##### **SFML Version**: 2.5.1+dfsg-2

##### **IDE**: VSCode

---

![game in action](/files/minesweeper.png)

## **Usage**

Follow the steps below to install the necessary tools, build the project, and run the game.
If you are currently taking UF's COP 3503, **DO NOT** view this codebase. You will be flagged for cheating and removed from the class immediately.

---

### **1. Install Required Tools**

#### **Install a C++ Compiler**

You need a C++ compiler that supports C++17. On Ubuntu, you can install `g++` by running:

```bash
sudo apt update
sudo apt install g++
```

#### **Install SFML**

The game uses the Simple and Fast Multimedia Library (SFML) for graphics and input handling. On Ubuntu, install SFML and `pkg-config` using:

```bash
sudo apt-get install libsfml-dev pkg-config
```

On macOS, the client uses the SFML 2 API. Install the compatible keg-only
formula and `pkgconf` using:

```bash
brew install sfml@2 pkgconf
```

The Makefile discovers either installation through `pkg-config`; no global
Homebrew symlink is required.

### **2. Build Project**

In the project directory, use the Makefile to compile the code:

```bash
make build
```

The code is split into three dependency layers:

```text
src/core    Headless game rules and timing; no SFML or networking
src/client  SFML input, windows, and rendering; depends on core
src/server  Command queue, authoritative games, and public snapshots; depends on core
```

Build the graphical client or headless runtime separately:

```bash
make client
make server
make test
```

### **3. Run the Game**

After building, if you would like to, edit the config file.
Go to files/board_config.cfg to change each line associated with rows, cols, num_mines respectively.

Finally, run with

```bash
./minesweeper-client
```

The headless runtime currently exposes a small development command adapter:

```bash
./minesweeper-server
```

It accepts `reveal ROW COL`, `flag ROW COL`, `restart`, and `quit`. A WebSocket
adapter can later parse network messages into the same `Command` queue without
giving networking code direct access to game state.

### Container and persistent data

Build and run the headless runtime with a persistent data volume:

```bash
docker build -t break-my-system/minesweeper .
docker run --rm -v minesweeper-data:/data break-my-system/minesweeper
```

The runtime reads these environment variables:

```text
MINESWEEPER_MODE=server
MINESWEEPER_PORT=7575
MINESWEEPER_DATA_DIR=/data
MINESWEEPER_LEADERBOARD_PATH=/data/leaderboard.csv
```

`MINESWEEPER_LEADERBOARD_PATH` overrides the file location directly. When it is
unset, the runtime writes `leaderboard.csv` below `MINESWEEPER_DATA_DIR`. The
directory is created on startup, and startup fails when the configured path is
not writable.

Server mode listens on `MINESWEEPER_PORT` using a private length-prefixed JSON
protocol. Each frame starts with a four-byte unsigned big-endian payload length,
followed by one UTF-8 JSON command. Socket I/O, partial frame buffering, command
dispatch, and event serialization run in one `poll()` loop; game behavior stays
inside the headless `Server` command queue.
