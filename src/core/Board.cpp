#include "core/Board.h"

#include <stdexcept>

Board::Board(int rows, int cols, int mines){
    if (rows <= 0 || cols <= 0 || mines < 0 || mines >= rows * cols) {
        throw std::invalid_argument("Invalid board dimensions or mine count");
    }

    // now need to go through each tile and set its neighbors array, and if its a pointer or not
    
    // create mineless board 
    _rows = rows;
    _cols = cols;
    _mines = mines;
    counter = mines;
    _state = 0; // -1: defeat, 0 still playing, 1: victory

    // set 2D vector of tiles (no mines) 
    create_empty_board();

    // place all mines on board
    randomize_mines(mines);

    // set boards neighbors
    set_board_neighbors();
}

// init functions

void Board::create_empty_board(){

    tile_vector.clear();

    // first, push back each row
    for (int r = 0; r < _rows; r++) {
        tile_vector.push_back(std::vector<Tile>()); // Add an empty row
    }

    // now, add columns  
    for(int r = 0; r < _rows; r += 1){
        for(int c = 0; c < _cols; c += 1){
            // initially set all times to no mine
            tile_vector[r].push_back(Tile(r, c));
        }
    } 
}

void Board::randomize_mines(int mines){

        // Create a random number generator engine
    std::random_device rd; // Obtain a random seed from the hardware
    std::mt19937 gen(rd()); // Mersenne Twister 19937 generator

    // Define a distribution (e.g., uniform integer distribution)
    std::uniform_int_distribution<int> cols_distribution(0, _cols - 1); // Range from 1 to 6 (inclusive)
    std::uniform_int_distribution<int> rows_distribution(0, _rows - 1);

    
    for(int i = 0; i < mines; i += 1){

        int rand_col, rand_row;

        do{
            rand_col = cols_distribution(gen);
            rand_row = rows_distribution(gen);
        }while(tile_vector[rand_row][rand_col].is_mine());
        // now, rand_col and rand_row are not a mine, so make it into a mine
        tile_vector[rand_row][rand_col].become_mine();
    }

}

void Board::set_board_neighbors(){
    // set each tiles neighbors
    for(int r = 0; r < _rows; r += 1){
        for (int c = 0; c < _cols; c += 1){
            tile_vector[r][c].setup_neighbors(tile_vector);
        }
    }
}


// getters 

int Board::board_state() const{
    // returns the boards stored state
    return _state;
}

bool Board::contains(int row, int col) const{
    return row >= 0 && row < _rows && col >= 0 && col < _cols;
}


// change board state

void Board::reveal_all(){ 
    // reveal every tile on board
    for(int row = 0; row < _rows; row += 1){
        for(int col = 0; col < _cols; col += 1){
            tile_vector[row][col].reveal();
        }
    }

}

void Board::reveal_mines(){
    // reveals all mines in board
    for(int r = 0; r < _rows; r += 1){
        for (int c = 0; c < _cols; c += 1){
            if (tile_vector[r][c].is_mine()){
                tile_vector[r][c].reveal(); // add mine 
            }
        }
    }
}

void Board::hide_mines(){
    // hides all mines in board

    for(int r = 0; r < _rows; r += 1){
        for (int c = 0; c < _cols; c += 1){
            if (tile_vector[r][c].is_mine()){
                tile_vector[r][c].hide();
            }
        }
    }

}

void Board::reset_board(){

    counter = _mines;
    _state = 0; // -1: defeat, 0 still playing, 1: victory

    create_empty_board();
    randomize_mines(_mines);
    set_board_neighbors();
} 

void Board::update_board(int row, int col, bool left_click){
    // determine if game state is changed by these updates!

    if (!contains(row, col) || _state != 0){
        return;
    }

    if (left_click){
        // left click: reveal

        int temp_state = tile_vector[row][col].left_click();
        
        // if temp state is -1, a mine was clicked on
        if(temp_state == -1){
            _state = -1;
        }


    }else{
        // right click: place/remove flag and also change counter
        counter -= tile_vector[row][col].right_click();
    }

    // check if game won (all non-mines revealed)
    if (check_game_won()){
        _state = 1;
    }
    
}

void Board::flag_all_mines(){
    // place flag on all mines
    for(int r = 0; r < _rows; r += 1){
        for(int c = 0; c < _cols; c += 1){
            if(tile_vector[r][c].is_mine() && !tile_vector[r][c].flag_placed()){
                tile_vector[r][c].place_flag();
            }
        }
    }

    counter = 0;
}

bool Board::check_game_won(){
    // checks if game won (all non mines revealed) 

    for(int r = 0; r < _rows; r += 1){
        for(int c = 0; c < _cols; c += 1){
            if (!tile_vector[r][c].is_mine() && !tile_vector[r][c].is_revealed()){
                // if tile isn't a mine and tile isnt revealed, game cannot be won
                return false;
            }

        }
    }

    // if every non-mine tile is revealed, return true!
    return true;
}

// debuggings stuff

void Board::print_board(){
    for(int r = 0; r < _rows; r += 1){
        for(int c = 0; c < _cols; c += 1){
            tile_vector[r][c].print_tile();
        }
    }
}
