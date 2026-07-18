#pragma once
#include <vector>
#include <iostream>


//Todo
/*

    //- make tiles with flags unrevealable
    // - not let revealed tiles to be flag placed
    - recursively reveal all other tiles if no mines surround tile
    //- randomly place mines
    - reveal if game is won 
    //- make left and right clicked actions to better organize stuff
    - draw mine on top of flag if mine revealed

*/


class Tile{

    bool _revealed, _is_mine, _has_flag;

    int _row, _col; // row,col location of tile

    Tile* neighbors[8];

    void init_variables(int row, int col); // setup all variables
    void set_neighbors_null(); // set all neighbors null

public:

    Tile(int row, int col);
    void setup_neighbors(std::vector<std::vector<Tile>> &board);

    void place_flag(){_has_flag = true;};

    // state changers
    int left_click(); // reveal
    int right_click(); // toggle flag

    void become_mine(); // turns tile into a mine
    void reveal(); // set tile state to reveal
    void hide(); // hide tile

    // getters 
    bool is_mine() const {return _is_mine;};
    bool flag_placed() const {return _has_flag;};
    int get_adjacent_mines() const; // returns number of hidden mines
    bool is_revealed() const {return _revealed;};

    // debugging
    void print_tile();
    void print_neighbors();

};
