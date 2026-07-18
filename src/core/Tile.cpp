#include "core/Tile.h"

Tile::Tile(int row, int col){

    init_variables(row, col); // init tiles variables
    set_neighbors_null(); // set all neighbors to nullptr first

}

// ============ init functions

void Tile::init_variables(int row, int col){

    // set the row, col of that tile 
    _row = row;
    _col = col;

    _is_mine = false;
    
    // set variables
    _revealed = false;
    _has_flag = false;

}

void Tile::set_neighbors_null(){
    // set all surrounding tiles to null (for now)
    for(int i = 0; i < 8; i += 1){
        neighbors[i] = nullptr;
    }
}

// ============ information getters

int Tile::get_adjacent_mines() const{
    // returns number of hidden mines by cycling through neighbords
    int num_adjacent = 0;

    for(int i = 0; i < 8; i += 1){
        if (neighbors[i] != nullptr && neighbors[i]->is_mine()){
            num_adjacent += 1;
        }
    }

    return num_adjacent;
}

void Tile::setup_neighbors(std::vector<std::vector<Tile>> &board){
    // using board and current location, setup 
    int max_rows = board.size();
    int max_cols = board[0].size();

    int placement = 0;
    
    for(int i = -1; i <= 1; i += 1 ){
        for(int j = -1; j <= 1; j += 1){
            if (i == 0 && j == 0){
                // skip the case where we look at our own tile
                continue;
            }
            int cur_row = _row + i;
            int cur_col = _col + j;
            
            if(cur_row < 0 || cur_row >= max_rows){
                // check if row out of bounds
                continue;

            }else if (cur_col < 0 || cur_col >= max_cols){
                // check if col out of bounds
                continue;
            }else{
                // still in board bounds, so point to it
                neighbors[placement] = &board[cur_row][cur_col];
            }

            // incriment placement
            placement += 1;
        }
    }
}

// ============ debugging

void Tile::print_tile(){
    std::cout << std::endl;
    std::cout << "revealed: " << _revealed << " is_mine: " << _is_mine << " has_flag: " << _has_flag << std::endl;
    std::cout << "_row, _col: (" << _row << ", " << _col << ")\n";
    std::cout << "neighbor mines: " << get_adjacent_mines() << std::endl; 
    std::cout << std::endl;
}

void Tile::print_neighbors(){
    //print info of each neighbor

    std::cout << "Printing neighbors of tile _row: " << _row << " _col: " << _col << std::endl;

    for(int i = 0; i < 8; i += 1){
        if (neighbors[i] != nullptr){
            neighbors[i]->print_tile();
        }
    }
}


// ============ State Changers ============

int Tile::left_click(){ 
    // return -1 if mine clicked, 0 if else

    // do nothing if tile has flag placed on it
    if (_has_flag){
        return 0;
    }

    // reveal the tile
    reveal();
    
    if (_is_mine){
        // if tile is a mine, return -1 (game lost)
        return -1;
    }

    // if tile is lonely (0 mine neighbors) reveal surrounding lonely tiles recursively
    if(get_adjacent_mines() == 0){
        // if tile is "lonely" reveal all nearby
        for(int i = 0; i < 8; i += 1){
            if (neighbors[i] != nullptr && (neighbors[i]->_is_mine == false)){
                // if this neighbor is not a null pointer, and is not a mine, reveal itself
                if (neighbors[i]->_revealed == false){
                    // if this neighbor is hidden and is a lonely tile, recursively call it 
                    neighbors[i]->left_click();
                }
            }
        }
    }

    return 0;

}

int Tile::right_click(){ 
    // return what to add to counter (1 if adding flag, -1 if removing flag)

    // if revealed, no flag is added
    if (_revealed){
        return 0;
    }

    int return_val = 0;
    if (_has_flag == true){
        // we are removing a flag
        return_val = -1;
    }else if (_has_flag == false){
        // we are adding a flag
        return_val = 1;
    }
    _has_flag = !_has_flag;

    return return_val;
}

void Tile::become_mine(){
    // set tile to mine
    _is_mine = true;
    // remove flag
    _has_flag = false;
}

void Tile::reveal(){
    // set tile state to reveal and remove flag
    _has_flag = false;
    _revealed = true; 
} 

void Tile::hide(){
    _revealed = false;
}
