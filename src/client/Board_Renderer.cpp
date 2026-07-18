#include "client/Board_Renderer.h"

Board_Renderer::Board_Renderer(Texture_Manager *manager){
    texture_manager = manager;
}

void Board_Renderer::draw_sprite(sf::RenderWindow &window, std::string texture_name, int row, int col){
    sf::Sprite sprite(texture_manager->getTexture(texture_name));
    sprite.setPosition(sf::Vector2f(col * 32, row * 32));
    window.draw(sprite);
}

void Board_Renderer::draw(sf::RenderWindow &window, const Board &board, bool masked, bool debugging){
    for(int row = 0; row < board.rows(); row += 1){
        for(int col = 0; col < board.cols(); col += 1){
            const Tile &tile = board.tile_at(row, col);

            if(masked){
                draw_sprite(window, "tile_revealed", row, col);
                continue;
            }

            if(!tile.is_revealed()){
                draw_sprite(window, "tile_hidden", row, col);

                if(tile.flag_placed()){
                    draw_sprite(window, "flag", row, col);
                }

                if(debugging && tile.is_mine()){
                    draw_sprite(window, "mine", row, col);
                }

                continue;
            }

            draw_sprite(window, "tile_revealed", row, col);

            if(tile.flag_placed()){
                draw_sprite(window, "flag", row, col);
            }

            if(tile.is_mine()){
                draw_sprite(window, "mine", row, col);
                continue;
            }

            int adjacent_mines = tile.get_adjacent_mines();
            if(adjacent_mines > 0){
                draw_sprite(window, "number_" + std::to_string(adjacent_mines), row, col);
            }
        }
    }
}
