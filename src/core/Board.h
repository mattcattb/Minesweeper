#pragma once
#include <random>
#include <vector>

#include "core/Tile.h"


//Todo
/*

    // - randomly place mines
    //- maybe smarter updating of each tiles sprite loader
    - check board state (won, lost, still playing)
    //- FIX NEIGHBOR MINES FUNCTION!!! HUGE ISSUE
*/


class Board{

    int _mines, _rows, _cols, _state;
    int counter; // what counter should display (starts at num mines)

    std::vector<std::vector<Tile>> tile_vector; // tile_vector[row][col]

    // initialize board and place mines
    void create_empty_board();
    void randomize_mines(int mines);
    void set_board_neighbors();

    bool check_game_won();


public:

    Board(int rows, int cols, int mines);
    Board(const Board&) = delete;
    Board& operator=(const Board&) = delete;
    Board(Board&&) = delete;
    Board& operator=(Board&&) = delete;
    
    // update a tile based on its board coordinates
    void update_board(int row, int col, bool left_click);

    // get information on board
    int board_state() const; // says if board lost, won, or neither
    int get_counter() const {return counter;}; // get board counter
    int rows() const {return _rows;};
    int cols() const {return _cols;};
    bool contains(int row, int col) const;
    const Tile& tile_at(int row, int col) const {return tile_vector[row][col];};

    // change board state
    void reveal_all(); // set every tile to revealed
    void reveal_mines(); // set mines to revealed 
    void hide_mines(); // set all mines to hidden
    void reset_board(); // resets board

    void flag_all_mines();

    // debugging
    void print_board(); // print all tiles in board

};
