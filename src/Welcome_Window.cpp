#include "Welcome_Window.h"


Welcome_Window::Welcome_Window(int rows, int cols){
    // constructor for Welcome Window

    init_variables(rows, cols);
    init_window();
    init_fonts();
    init_text();

}

int Welcome_Window::event_loop(){
    // return -1, end everything
    // return 0, keep the loop
    // return 1, go to next window (game)

    while (render_window.isOpen()){
        // draw board, and then poll event
        sf::Event event;
        while(render_window.pollEvent(event)){
            char c;

            switch (event.type)
            {

            case sf::Event::Closed:
                // close window! return from event loop with -1 (say to end everything)
                render_window.close();
                
                return -1;
                
            case sf::Event::TextEntered:
                // text entered case!
                c = event.text.unicode;
                if((event.text.unicode == 8) && (character_name.length() > 0)){
                    // enter backspace and string not empty, so delete 1 character
                    character_name.pop_back(); 
                
                
                } else if (((c <= 'z' && c >= 'a') || (c <= 'Z' && c >= 'A')) && character_name.length() < 10){
                    // uppercase or lowercase letter entered AND there is enough space
                    c = std::tolower(c); // convert to lower
                    if (character_name.length() == 0){
                        // if this is the first letter, capitalize it 
                        c = std::toupper(c);
                    }
                    
                    // add letter to character name
                    character_name.push_back(c);

                }

                if (event.text.unicode == 13 && character_name.length() > 0){
                    // enter key pressed and character length is greater then 0, go into game mode
                    render_window.close();
                    return 1;
                }

                update_username();
                break;

            default:
                break;
            }
        }

        // clear all 
        render_window.clear(sf::Color::Blue);

        // draw and display all!
        draw_all();

        // display all!
        render_window.display();
    }

    return 0;

}

std::string Welcome_Window::get_character_name(){
    return character_name;
}


// ========= INIT HELPER FUNCTIONS ==========

void Welcome_Window::init_fonts(){
    if(! _text_font.loadFromFile("files/font.ttf")){
        std::cout << "error loading font\n";
    }
}


void Welcome_Window::init_window(){
    // init window!
    render_window.create(sf::VideoMode(_width, _height), "welcome", sf::Style::Close);

}


void Welcome_Window::init_variables(int rows, int cols){
    // init all variables

    _height = rows*32 + 100;
    _width = cols*32;
    _cols = cols;
    _rows = rows;

}

void Welcome_Window::init_text(){
    // setup and place all text values

    // placements of all text
    sf::Vector2f welcome_pos(_width/2.0, (_height/2.0)-150);
    sf::Vector2f name_prompt_pos(_width/2.0, (_height/2.0)-75);
    sf::Vector2f user_name_pos(_width/2.0, _height/2.0 - 45);

    // actually set this with the helpers
    setup_text(_welcome_message, "WELCOME TO MINESWEEPER", _text_font, true, true, sf::Color::White, 24, welcome_pos);
    setup_text(_enter_name_prompt, "Enter your name:", _text_font, true, false, sf::Color::White, 20, name_prompt_pos);
    setup_text(_user_name, "|", _text_font, true, false, sf::Color::Yellow, 18, user_name_pos);

}


void Welcome_Window::draw_all(){
    // draw everything in the window
    draw_text();
}

// ============== Helper Functions ==============

void Welcome_Window::setup_text(sf::Text &text, std::string message, sf::Font &font, bool bold, bool underlined, sf::Color color, int size, sf::Vector2f position){
    // helper for quickly setting up text

    // select the font
    text.setFont(font); // font is a sf::Font

    // set the string to display
    text.setString(message);

    // set the character size
    text.setCharacterSize(size); // in pixels, not points!

    // set the color
    text.setFillColor(color);

    // set bold or underlined if needed
    if (bold){text.setStyle(sf::Text::Bold);};
    if (underlined){text.setStyle(sf::Text::Underlined);};

    // move position
    setTextPos(text, position.x, position.y);

}

void Welcome_Window::setTextPos(sf::Text &text, float x, float y){
    sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin(textRect.left + textRect.width/2.0f,
                    textRect.top+textRect.height/2.0f);
    text.setPosition(sf::Vector2f(x,y));
}

void Welcome_Window::draw_text(){
    render_window.draw(_welcome_message);
    render_window.draw(_enter_name_prompt);
    render_window.draw(_user_name);

}

void Welcome_Window::update_username(){
    // update username?
    std::string final_user = character_name + "|";
    _user_name.setString(final_user);
    setTextPos(_user_name, _width/2.0, _height/2.0 - 45);
}
