#include "ui/board_renderer.hpp"

namespace minesweeper::ui {

BoardRenderer::BoardRenderer(TextureManager *manager){
    texture_manager = manager;
}

void BoardRenderer::draw_sprite(sf::RenderWindow &window, std::string texture_name, int row, int col){
    sf::Sprite sprite(texture_manager->getTexture(texture_name));
    sprite.setPosition(sf::Vector2f(col * 32, row * 32));
    window.draw(sprite);
}

void BoardRenderer::draw(sf::RenderWindow &window, const minesweeper::Board &board, bool masked, bool debugging){
    for(int row = 0; row < board.rows; row += 1){
        for(int col = 0; col < board.cols; col += 1){
            const minesweeper::Position position{row, col};
            const minesweeper::Tile &tile = minesweeper::tile_at(board, position);

            if(masked){
                draw_sprite(window, "tile_revealed", row, col);
                continue;
            }

            if(!tile.revealed){
                draw_sprite(window, "tile_hidden", row, col);

                if(tile.flagged){
                    draw_sprite(window, "flag", row, col);
                }

                if(debugging && tile.mine){
                    draw_sprite(window, "mine", row, col);
                }

                continue;
            }

            draw_sprite(window, "tile_revealed", row, col);

            if(tile.flagged){
                draw_sprite(window, "flag", row, col);
            }

            if(tile.mine){
                draw_sprite(window, "mine", row, col);
                continue;
            }

            const int mine_count = minesweeper::adjacent_mines(board, position);
            if(mine_count > 0){
                draw_sprite(window, "number_" + std::to_string(mine_count), row, col);
            }
        }
    }
}

}  // namespace minesweeper::ui
