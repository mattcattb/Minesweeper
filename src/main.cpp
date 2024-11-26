#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Welcome_Window.h"
#include "Game_Window.h"
#include "Texture_Manager.h"
#include "Leaderboard_Window.h"

/* CHECKLIST

    //- Board_config changes board size and mines number
    //- Tile Display (unrevealed, revealed and number, or mine)
    //- Tile revealing (clicking on tile revealed, if mine game over) 
    //if 0 adjacent mines, revealeach of those lonely tiles
    // - right clicking hidden tile toggles flag. remove flag to reveal
    //- counter = mines on board - flags placed
    //- Victory state: revealing all non mines ends game
    //- Defeat state: clicking mine ends game. all mines revelaed
    //- Random mine: game stat when resetting the mines forkm cfg file 
    //are randomly placed on map
    //- Welcome window made
    //- timer/pause button: timer counts up. clicking pause pauses counter 
    // and shows all tiles with tile_revealed png. Only leaderboard & face buttons work in puase mode.
    // - leaderboard button: shows top 5 leaders. pauses timer and changes all to tile_revealed

    - BUG FIXES!!!

*/

void read_config(int &cols, int &rows, int &mines);

int main(int argc, char ** argv){

    int cols, rows, mines;
    read_config(rows, cols, mines); // read in config info from file 

    int state = 0;
    std::string username; // username of this character

    Welcome_Window welcome_window(rows, cols);

    // welcome window
    state = welcome_window.event_loop(); 
    if (state == -1){
        // immediatly end program
        return 0;
    }
    
    // go into game part now
    username = welcome_window.get_character_name();
 
    Game_Window game_window(rows, cols, mines, username);
    state = game_window.event_loop();

    // game loop
    return 0;
}

void read_config(int &rows, int &cols, int &mines){
    
    std::ifstream config_fin("files/board_config.cfg");
    std::string buffer;
    
    config_fin >> buffer;
    cols = stoi(buffer);
    
    config_fin >> buffer; 
    rows = stoi(buffer);

    config_fin >> buffer;
    mines = stoi(buffer);

}

