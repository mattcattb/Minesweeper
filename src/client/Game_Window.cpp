#include "client/Game_Window.h"

Game_Window::Game_Window(int rows, int cols, int mines, std::string username){

    // setup board!
    // initialize parts of display
    texture_manager = new Texture_Manager();
    init_variables(rows, cols, mines, username);
    init_buttons();
    init_window();
    init_displays();
    init_game();
    leaderboard = new Leaderboard_Window(rows, cols);
    
}

Game_Window::~Game_Window(){
    
    delete game;
    delete board_renderer;
    delete counter;
    delete minutes_timer;
    delete seconds_timer;
    delete leaderboard;
    delete texture_manager;

}

int Game_Window::event_loop(){

    // returns -1 to say the window is closed, 1 to say to switch to game view

    while (render_window.isOpen())
    {
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
                    GameStatus previous_status = game->status();
                    int row = mouse_pos.y / 32;
                    int col = mouse_pos.x / 32;

                    if (left_click){
                        game->reveal(row, col);
                    }else{
                        game->toggle_flag(row, col);
                    }

                    int counter_val = game->board().get_counter();
                    counter->set_display(counter_val);

                    if (previous_status == GameStatus::Playing && game_won()){
                        // set face to sunglasses face
                        happy_button.setTexture(texture_manager->getTexture("face_win"));

                        // draw everything!
                        draw_all();
                        render_window.display();

                        // add users score to place
                        int mins = game->elapsed_seconds() / 60;
                        int secs = game->elapsed_seconds() % 60;
                        
                        // if score not on the leaderboard, add it and display leaderboard
                        leaderboard->add_score(_username, mins, secs);
                        leaderboard->display_leaderboard();
                    }else if (previous_status == GameStatus::Playing && game->status() == GameStatus::Lost){
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

        update_time();

        // draw everything
        draw_all();

        render_window.display();

    }

    return 0;
} 

// =========== init functions ===========

void Game_Window::init_game(){
    game = new Game(_rows, _cols, _mines);
    board_renderer = new Board_Renderer(texture_manager);
    
}

void Game_Window::init_window(){
    render_window.create(sf::VideoMode(_width, _height), "game", sf::Style::Close);
}

void Game_Window::init_variables(int rows, int cols, int mines, std::string username){
    _username = username;
    _cols = cols;
    _rows = rows;
    _mines = mines;
    _height = _rows*32 + 100;
    _width = _cols*32;
    debugging = false;
    board_masked = false;
}

void Game_Window::init_displays(){
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
    counter = new Display(array_size, counter_x, counter_y, digits);

    counter->set_display(_mines);

    // init minutes and seconds timer

    int minutes_x = _cols*32 - 97;
    int minutes_y = 32*(_rows+0.5) + 16;

    int seconds_x = _cols*32 - 54;
    int seconds_y = 32*(_rows+0.5) + 16; 

    minutes_timer = new Display(2, minutes_x, minutes_y, digits, false);
    seconds_timer = new Display(2, seconds_x, seconds_y, digits, false);

    minutes_timer->set_display(0);
    seconds_timer->set_display(0);

}

void Game_Window::init_buttons(){

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

void Game_Window::update_pause_button(){
    // make pause button have each tile be "revealed" or switch back to normal state
    
    game->toggle_paused();
    board_masked = game->paused();

    // change pause sprite
    if (game->paused()){
        // went from unpaused -> paused
        pause_play_button.setTexture(texture_manager->getTexture("play"));
    }else{
        // went from paused -> unpaused
        pause_play_button.setTexture(texture_manager->getTexture("pause"));
    }
}

void Game_Window::update_leaderboard(){

    // mask all tiles (put hidden sprite on top)

    if(!game_won()){
        bool was_paused = game->paused();
        bool was_masked = board_masked;
        game->set_paused(true);
        board_masked = true;
        draw_all();

        render_window.display();
        
        //display leaderboard window
        leaderboard->display_leaderboard();

        // when leaderboard window closed, return the board to its previous display state
        game->set_paused(was_paused);
        board_masked = was_masked;

    }else{
        leaderboard->display_leaderboard();
    }
}

void Game_Window::update_debug_button(){
    //make not work when game is in loss or win state
   
    // toggle debug state, either showing all mines or hiding all mines
    debugging = !debugging;

    // button should not work when game ends (victory/loss)
}

void Game_Window::update_happy_face_button(){

    // happyface button was clicked!
    // restart and re-randomize board
    game->restart();
    debugging = false;
    board_masked = false;

    init_buttons();

    // set counter to new counter val
    int counter_val = game->board().get_counter();
    counter->set_display(counter_val);
    minutes_timer->set_display(0);
    seconds_timer->set_display(0);
    
}

void Game_Window::update_time(){
    int elapsed_seconds = game->elapsed_seconds();
    minutes_timer->set_display(elapsed_seconds / 60);
    seconds_timer->set_display(elapsed_seconds % 60);
}

// =========== draw functions ===========

void Game_Window::draw_all(){

    render_window.clear(sf::Color::White);

    draw_buttons();
    draw_board();
    draw_displays();

}

void Game_Window::draw_buttons(){

    render_window.draw(happy_button);
    render_window.draw(debug_button);
    render_window.draw(pause_play_button);
    render_window.draw(leaderboard_button);

}

void Game_Window::draw_displays(){

    // draw each digit in counter sprite array
    counter->draw(render_window);
    minutes_timer->draw(render_window);
    seconds_timer->draw(render_window);
}

void Game_Window::draw_board(){
    board_renderer->draw(render_window, game->board(), board_masked, debugging);
}


// ======== helpers ==========

bool Game_Window::tile_clicked(sf::Vector2i &mouse_pos){

    if (mouse_pos.x >= 0 && mouse_pos.x < _cols*32 &&
        mouse_pos.y >= 0 && mouse_pos.y < _rows*32){
        return true;
    }else{
        return false;
    }    

}

bool Game_Window::pause_button_clicked(sf::Vector2i &mouse_pos){
    
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

bool Game_Window::leaderboard_button_clicked(sf::Vector2i &mouse_pos){
    
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

bool Game_Window::debug_button_clicked(sf::Vector2i &mouse_pos){

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

bool Game_Window::happy_face_button_clicked(sf::Vector2i &mouse_pos){
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
