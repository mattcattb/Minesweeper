#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>

namespace minesweeper::ui {

struct DigitDisplay{


    sf::Sprite * display_array;
    sf::Texture * digits_texture;

    int left_x, left_y;
    int display_size;
    bool negative;

    void set_digit(sf::Sprite &digit, int num);

    DigitDisplay(int digits, int x, int y, sf::Texture * digits_texture, bool n=true);
    ~DigitDisplay(){
        delete[] display_array;
    }

    void set_display(int val); // sets display to this value
    void draw(sf::RenderWindow &window); // draw all digits

};

}  // namespace minesweeper::ui
