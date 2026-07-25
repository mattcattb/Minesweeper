#pragma once
#include <SFML/Graphics.hpp>
#include <string>

#include "game.hpp"
#include "ui/texture_manager.hpp"

namespace minesweeper::ui {

class BoardRenderer{
    TextureManager * texture_manager;

    void draw_sprite(sf::RenderWindow &window, std::string texture_name, int row, int col);

public:
    BoardRenderer(TextureManager *manager);
    void draw(sf::RenderWindow &window, const Board &board, bool masked, bool debugging);
};

}  // namespace minesweeper::ui
