#include "ui/game_window.hpp"

#include <utility>

namespace minesweeper::ui {

GameWindow::GameWindow(int rows, int cols, int mines, std::string username){

    // setup board!
    // initialize parts of display
    texture_manager = new TextureManager();
    init_variables(rows, cols, mines, username);
    init_buttons();
    init_window();
    init_displays();
    init_game();
    leaderboard = new LeaderboardWindow(rows, cols);
    
}

GameWindow::~GameWindow(){
    
    delete board_renderer;
    delete counter;
    delete minutes_timer;
    delete seconds_timer;
    delete leaderboard;
    delete texture_manager;

}

int GameWindow::event_loop(){

    // returns -1 to say the window is closed, 1 to say to switch to game view

    while (render_window.isOpen())
    {
        update_time();

        sf::Event event;
        
        while(render_window.pollEvent(event)){
            bool left_click;
            sf::Vector2i mouse_pos;

            switch (event.type)
            {
            
            case sf::Event::Closed:
                // window closed!
                render_window.close();
                
                return -1;

                break;
                            
            case sf::Event::MouseButtonPressed:
                
                // get mouse position of pressed mouse
                mouse_pos = sf::Mouse::getPosition(render_window);
                

                // get if left or right click
                left_click = (event.mouseButton.button == sf::Mouse::Left);

                // Convert UI input into game commands. The window never mutates the board directly.
                if (tile_clicked(mouse_pos) && !game_stopped()){
                    const minesweeper::GameStatus previous_status = game.status;
                    int row = mouse_pos.y / 32;
                    int col = mouse_pos.x / 32;

                    if (left_click){
                        minesweeper::reveal(game, {row, col});
                    }else{
                        minesweeper::toggle_flag(game, {row, col});
                    }

                    int counter_val = minesweeper::remaining_mines(game.board);
                    counter->set_display(counter_val);

                    if (previous_status == minesweeper::GameStatus::Playing && game_won()){
                        // set face to sunglasses face
                        happy_button.setTexture(texture_manager->getTexture("face_win"));

                        // draw everything!
                        draw_all();
                        render_window.display();

                        // add users score to place
                        const int elapsed_seconds =
                            static_cast<int>(game.elapsed_milliseconds / 1000);
                        int mins = elapsed_seconds / 60;
                        int secs = elapsed_seconds % 60;
                        
                        // if score not on the leaderboard, add it and display leaderboard
                        leaderboard->add_score(_username, mins, secs);
                        leaderboard->display_leaderboard();
                    }else if (previous_status == minesweeper::GameStatus::Playing &&
                        game.status == minesweeper::GameStatus::Lost){
                        happy_button.setTexture(texture_manager->getTexture("face_lose"));
                    }

                }else if (pause_button_clicked(mouse_pos) && !game_stopped()){
                    // if pause clicked & game still running, update pause button
                    update_pause_button();
                    
                }else if(leaderboard_button_clicked(mouse_pos)){
                    update_leaderboard();

                }else if(debug_button_clicked(mouse_pos) && !game_stopped()){
                    // if debug button clicked and game still going, update debug button
                    update_debug_button();

                }else if(happy_face_button_clicked(mouse_pos)){
                    update_happy_face_button(); 
                }

                break;

            default:
                break;
            }
        }

        // draw everything
        draw_all();

        render_window.display();

    }

    return 0;
} 

// =========== init functions ===========

void GameWindow::init_game(){
    const auto mines = minesweeper::choose_mines(
        _rows, _cols, _mines, random_generator);
    game = minesweeper::create_game(_rows, _cols, mines);
    last_time_update = std::chrono::steady_clock::now();
    board_renderer = new BoardRenderer(texture_manager);
    
}

void GameWindow::init_window(){
    render_window.create(sf::VideoMode(_width, _height), "game", sf::Style::Close);
}

void GameWindow::init_variables(int rows, int cols, int mines, std::string username){
    _username = username;
    _cols = cols;
    _rows = rows;
    _mines = mines;
    _height = _rows*32 + 100;
    _width = _cols*32;
    debugging = false;
    board_masked = false;
}

void GameWindow::init_displays(){
    // initialize display elements (counter, timer)


    // init counter first, find how many digits of the mine there are
    int temp_mine = _mines;
    int array_size = 0;  
    while (temp_mine > 0){
        array_size += 1;
        temp_mine = temp_mine/10;
    }

    // array size is the number of digits in mine
    int counter_x = 32;
    int counter_y = 32*(_rows+0.5) + 16;
    sf::Texture * digits = &texture_manager->getTexture("digits");
    counter = new DigitDisplay(array_size, counter_x, counter_y, digits);

    counter->set_display(_mines);

    // init minutes and seconds timer

    int minutes_x = _cols*32 - 97;
    int minutes_y = 32*(_rows+0.5) + 16;

    int seconds_x = _cols*32 - 54;
    int seconds_y = 32*(_rows+0.5) + 16; 

    minutes_timer = new DigitDisplay(2, minutes_x, minutes_y, digits, false);
    seconds_timer = new DigitDisplay(2, seconds_x, seconds_y, digits, false);

    minutes_timer->set_display(0);
    seconds_timer->set_display(0);

}

void GameWindow::init_buttons(){

    // happy face button (cols/2.0 *32) - 32, 32*(rows+0.5)
    happy_button.setTexture(texture_manager->getTexture("face_happy"));
    happy_button.setPosition((_cols/2.0 * 32)-32, 32*(_rows+0.5));

    // debug button
    debug_button.setTexture(texture_manager->getTexture("debug"));
    debug_button.setPosition((_cols*32)-304, 32*(_rows+0.5));

    // leaderboard_texture
    leaderboard_button.setTexture(texture_manager->getTexture("leaderboard"));
    leaderboard_button.setPosition(_cols*32-176,(_rows+0.5)*32);

    // pause play button
    pause_play_button.setTexture(texture_manager->getTexture("pause"));
    pause_play_button.setPosition(_cols*32-240, (_rows+0.5)*32);

}

// =========== update functions =========

void GameWindow::update_pause_button(){
    // make pause button have each tile be "revealed" or switch back to normal state
    
    minesweeper::set_paused(game, !game.paused);
    board_masked = game.paused;
    last_time_update = std::chrono::steady_clock::now();

    // change pause sprite
    if (game.paused){
        // went from unpaused -> paused
        pause_play_button.setTexture(texture_manager->getTexture("play"));
    }else{
        // went from paused -> unpaused
        pause_play_button.setTexture(texture_manager->getTexture("pause"));
    }
}

void GameWindow::update_leaderboard(){

    // mask all tiles (put hidden sprite on top)

    if(!game_won()){
        bool was_paused = game.paused;
        bool was_masked = board_masked;
        minesweeper::set_paused(game, true);
        board_masked = true;
        draw_all();

        render_window.display();
        
        //display leaderboard window
        leaderboard->display_leaderboard();

        // when leaderboard window closed, return the board to its previous display state
        minesweeper::set_paused(game, was_paused);
        board_masked = was_masked;
        last_time_update = std::chrono::steady_clock::now();

    }else{
        leaderboard->display_leaderboard();
    }
}

void GameWindow::update_debug_button(){
    //make not work when game is in loss or win state
   
    // toggle debug state, either showing all mines or hiding all mines
    debugging = !debugging;

    // button should not work when game ends (victory/loss)
}

void GameWindow::update_happy_face_button(){

    // happyface button was clicked!
    // restart and re-randomize board
    const auto mines = minesweeper::choose_mines(
        _rows, _cols, _mines, random_generator);
    minesweeper::GameState replacement =
        minesweeper::create_game(_rows, _cols, mines);
    minesweeper::restart(game, std::move(replacement.board));
    last_time_update = std::chrono::steady_clock::now();
    debugging = false;
    board_masked = false;

    init_buttons();

    // set counter to new counter val
    int counter_val = minesweeper::remaining_mines(game.board);
    counter->set_display(counter_val);
    minutes_timer->set_display(0);
    seconds_timer->set_display(0);
    
}

void GameWindow::update_time(){
    const auto now = std::chrono::steady_clock::now();
    if (game.status == minesweeper::GameStatus::Playing && !game.paused) {
        game.elapsed_milliseconds +=
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_time_update).count();
    }
    last_time_update = now;

    const int elapsed_seconds =
        static_cast<int>(game.elapsed_milliseconds / 1000);
    minutes_timer->set_display(elapsed_seconds / 60);
    seconds_timer->set_display(elapsed_seconds % 60);
}

// =========== draw functions ===========

void GameWindow::draw_all(){

    render_window.clear(sf::Color::White);

    draw_buttons();
    draw_board();
    draw_displays();

}

void GameWindow::draw_buttons(){

    render_window.draw(happy_button);
    render_window.draw(debug_button);
    render_window.draw(pause_play_button);
    render_window.draw(leaderboard_button);

}

void GameWindow::draw_displays(){

    // draw each digit in counter sprite array
    counter->draw(render_window);
    minutes_timer->draw(render_window);
    seconds_timer->draw(render_window);
}

void GameWindow::draw_board(){
    board_renderer->draw(render_window, game.board, board_masked, debugging);
}


// ======== helpers ==========

bool GameWindow::tile_clicked(sf::Vector2i &mouse_pos){

    if (mouse_pos.x >= 0 && mouse_pos.x < _cols*32 &&
        mouse_pos.y >= 0 && mouse_pos.y < _rows*32){
        return true;
    }else{
        return false;
    }    

}

bool GameWindow::pause_button_clicked(sf::Vector2i &mouse_pos){
    
    sf::FloatRect pause_bounds = pause_play_button.getLocalBounds();
    float spriteWidth = pause_bounds.width; // x
    float spriteHeight = pause_bounds.height; // y 
    sf::Vector2f sprite_pos = pause_play_button.getPosition(); // top left 
    if (mouse_pos.x >= sprite_pos.x && mouse_pos.x <= (sprite_pos.x + spriteWidth) 
        && mouse_pos.y >= sprite_pos.y && mouse_pos.y <= (sprite_pos.y + spriteHeight)){
        return true;
    }else{
        return false;
    }
}

bool GameWindow::leaderboard_button_clicked(sf::Vector2i &mouse_pos){
    
    sf::FloatRect leaderboard_bounds = leaderboard_button.getLocalBounds();
    float spriteWidth = leaderboard_bounds.width; // x
    float spriteHeight = leaderboard_bounds.height; // y 
    sf::Vector2f sprite_pos = leaderboard_button.getPosition(); // top left 
    if (mouse_pos.x >= sprite_pos.x && mouse_pos.x <= (sprite_pos.x + spriteWidth) 
        && mouse_pos.y >= sprite_pos.y && mouse_pos.y <= (sprite_pos.y + spriteHeight)){
        return true;
    }else{
        return false;
    }
}

bool GameWindow::debug_button_clicked(sf::Vector2i &mouse_pos){

    sf::FloatRect debug_bounds = debug_button.getLocalBounds();
    float spriteWidth = debug_bounds.width; // x
    float spriteHeight = debug_bounds.height; // y 
    sf::Vector2f sprite_pos = debug_button.getPosition(); // top left 
    if (mouse_pos.x >= sprite_pos.x && mouse_pos.x <= (sprite_pos.x + spriteWidth) 
        && mouse_pos.y >= sprite_pos.y && mouse_pos.y <= (sprite_pos.y + spriteHeight)){
        return true;
    }else{
        return false;
    }
}

bool GameWindow::happy_face_button_clicked(sf::Vector2i &mouse_pos){
    sf::FloatRect debug_bounds = happy_button.getLocalBounds();
    float spriteWidth = debug_bounds.width; // x
    float spriteHeight = debug_bounds.height; // y 
    sf::Vector2f sprite_pos = happy_button.getPosition(); // top left 
    if (mouse_pos.x >= sprite_pos.x && mouse_pos.x <= (sprite_pos.x + spriteWidth) 
        && mouse_pos.y >= sprite_pos.y && mouse_pos.y <= (sprite_pos.y + spriteHeight)){
        return true;
    }else{
        return false;
    }
}

}  // namespace minesweeper::ui
