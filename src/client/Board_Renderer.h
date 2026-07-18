#pragma once
#include <SFML/Graphics.hpp>
#include <string>

#include "client/Texture_Manager.h"
#include "core/Board.h"

class Board_Renderer{
    Texture_Manager * texture_manager;

    void draw_sprite(sf::RenderWindow &window, std::string texture_name, int row, int col);

public:
    Board_Renderer(Texture_Manager *manager);
    void draw(sf::RenderWindow &window, const Board &board, bool masked, bool debugging);
};
