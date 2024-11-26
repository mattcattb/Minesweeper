#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>

class Welcome_Window{

    // variables
    int _height, _width;
    int _rows, _cols;

    std::string character_name;

    // text boxes to display
    sf::Text _welcome_message; // WELCOME TO MINESWEEPER
    sf::Text _enter_name_prompt; // enter name
    sf::Text _user_name;
    
    // assets  
    sf::Font _text_font;
    
    // render window (IMPORTANT)
    sf::RenderWindow render_window;

    // init welcome window objects
    void init_fonts();
    void init_window();
    void init_variables(int rows, int cols);
    void init_text();

    // helpers
    void setup_text(sf::Text &text, std::string message, sf::Font &font, bool bold, bool underlined, sf::Color color, int size, sf::Vector2f position);
    void setTextPos(sf::Text &text, float x, float y); // set position of text

    // drawing
    void draw_all();
    void draw_text();

    void update_username();

public:

    Welcome_Window(int rows, int cols);
    int event_loop(); // returns -1 to say the window is closed, 1 to say to switch to game view
    std::string get_character_name();

};

