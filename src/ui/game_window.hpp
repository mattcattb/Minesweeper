#pragma once

#include <SFML/Graphics.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <string>

#include "game.hpp"
#include "ui/board_renderer.hpp"
#include "ui/digit_display.hpp"
#include "ui/leaderboard_window.hpp"
#include "ui/texture_manager.hpp"

namespace minesweeper::ui {

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


class GameWindow{


    // === game and window variables
    int _height, _width, _mines;
    int _cols, _rows;
    bool debugging;
    bool board_masked;

    std::string _username;

    // === game objects
    GameState game;
    std::mt19937 random_generator{std::random_device{}()};
    std::chrono::steady_clock::time_point last_time_update;
    BoardRenderer *board_renderer;
    TextureManager* texture_manager;
    sf::RenderWindow render_window;
    LeaderboardWindow * leaderboard;


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
    DigitDisplay * counter;
    DigitDisplay * minutes_timer;
    DigitDisplay * seconds_timer;
    
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

    bool game_won() const {return game.status == GameStatus::Won;};
    bool game_stopped() const {return game.status != GameStatus::Playing;};

public:

    GameWindow(int rows, int cols, int mines, std::string username);
    ~GameWindow();

    int event_loop(); // returns -1 to say the window is closed, 1 to say to switch to game view

};

}  // namespace minesweeper::ui
