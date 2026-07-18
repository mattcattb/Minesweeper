#pragma once

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

#include "client/Board_Renderer.h"
#include "client/Display.h"
#include "client/Leaderboard_Window.h"
#include "client/Texture_Manager.h"
#include "core/Game.h"


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
    bool debugging;
    bool board_masked;

    std::string _username;

    // === game objects
    Game *game;
    Board_Renderer *board_renderer;
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
    void init_game();
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
    // === helpers
    bool tile_clicked(sf::Vector2i &mouse_pos); // bool if tile clicked
    bool pause_button_clicked(sf::Vector2i &mouse_pos); 
    bool leaderboard_button_clicked(sf::Vector2i &mouse_pos);
    bool debug_button_clicked(sf::Vector2i &mouse_pos);
    bool happy_face_button_clicked(sf::Vector2i &mouse_pos);

    bool game_won() const {return game->status() == GameStatus::Won;};
    bool game_stopped() const {return game->status() != GameStatus::Playing;};

public:

    Game_Window(int rows, int cols, int mines, std::string username);
    ~Game_Window();

    int event_loop(); // returns -1 to say the window is closed, 1 to say to switch to game view

};
