#include <SFML/Graphics.hpp>
#include <chrono>
#include <string>
#include <iostream>

#include "Board.h"
#include "Texture_Manager.h"
#include "Display.h"
#include "Leaderboard_Window.h"


//Todo
/*

    - make all share the same texture pointer, making speedup significantly
    //- setup digits counter
    //- integrate timer counter
    // - impliment leaderboard button
    //- toggle pause button
    - make debug button not remove flags?
    //- have reset also reset timer

*/


class Game_Window{


    // === game and window variables
    int _height, _width, _mines;
    int _cols, _rows;
    bool paused;
    bool debugging;

    std::string _username;

    // === time variables
    std::chrono::high_resolution_clock::time_point prev; // prev time point
    std::chrono::high_resolution_clock::time_point now; // past time point
    std::chrono::high_resolution_clock::time_point duration;
    
    std::chrono::seconds total_seconds_elapsed_time; // stored to be displayed as time passes

    // === game objects
    Board *board;
    Texture_Manager* texture_manager;
    sf::RenderWindow render_window;
    Leaderboard_Window * leaderboard;


    // === Buttons

    // happy face button
    sf::Sprite happy_button;
    // debug button
    sf::Sprite debug_button;
    // pause/play button
    sf::Sprite pause_play_button;
    // leaderboard button
    sf::Sprite leaderboard_button;
    
    // === Displays
    Display * counter;
    Display * minutes_timer;
    Display * seconds_timer;
    
    // === init functions
    void init_board();
    void init_window();
    void init_variables(int rows, int cols, int mines, std::string username);
    void init_displays();
    void init_buttons();

    // === update functions 
    void update_pause_button();
    void update_leaderboard();
    void update_debug_button();
    void update_happy_face_button();

    void update_time();

    // === draw functions
    void draw_all();
    void draw_buttons();
    void draw_displays();
    void draw_board();
    void draw_mask();

    // === helpers
    bool tile_clicked(sf::Vector2i &mouse_pos); // bool if tile clicked
    bool pause_button_clicked(sf::Vector2i &mouse_pos); 
    bool leaderboard_button_clicked(sf::Vector2i &mouse_pos);
    bool debug_button_clicked(sf::Vector2i &mouse_pos);
    bool happy_face_button_clicked(sf::Vector2i &mouse_pos);

    bool game_won(){return board->board_state()==1;};
    bool game_lost(){return board->board_state()==-1;};
    bool game_stopped(){return (game_won() || game_lost());};

public:

    Game_Window(int rows, int cols, int mines, std::string username);
    ~Game_Window();

    int event_loop(); // returns -1 to say the window is closed, 1 to say to switch to game view

};