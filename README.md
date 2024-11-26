# Minesweeper - Fall 2023 Project

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
You need a C++ compiler that supports at least C++11. On Ubuntu, you can install `g++` by running:
```bash
sudo apt update
sudo apt install g++
```
#### **Install SFML**
The game uses the Simple and Fast Multimedia Library (SFML) for graphics and input handling. Install it using:

```bash
sudo apt-get install libsfml-dev
```

### **2. Build Project**
In the project directory, use the Makefile to compile the code:

```bash
make build
```

### **3. Run the Game**
After building, if you would like to, edit the config file.
Go to files/board_config.cfg to change each line associated with rows, cols, num_mines respectively.

Finally, run with 
```bash
./project3.out
```
