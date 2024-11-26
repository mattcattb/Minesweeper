#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include <random>

#include "Tile.h"
#include "Texture_Manager.h"


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

    Texture_Manager * texture_manager;

    // initialize board and place mines
    void create_empty_board();
    void randomize_mines(int mines);
    void set_board_neighbors();

    bool check_game_won();


public:

    Board(int rows, int cols, int mines, Texture_Manager *manager);
    
    // update each tiles states based on mouse_position
    void update_board(sf::Vector2i mouse_pos, bool left_click);

    // get information on board
    int board_state(); // says if board lost, won, or neither
    int get_counter(){return counter;}; // get board counter

    // change board state
    void reveal_all(); // set every tile to revealed
    void reveal_mines(); // set mines to revealed 
    void hide_mines(); // set all mines to hidden
    void reset_board(); // resets board

    void flag_all_mines();

    void mask(); // adds empty tile
    void unmask(); // removes empty tile

    void toggle_debug_state(); // toggles debug state for each tile
    // draw board
    void draw_tiles(sf::RenderWindow &window); // draw each tile in board

    // debugging
    void print_board(); // print all tiles in board

};

