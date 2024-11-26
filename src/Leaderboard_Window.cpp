#include "Leaderboard_Window.h"

// ====== Initialization ======

Leaderboard_Window::Leaderboard_Window(int r, int c){
    
    _leaderboard_filename = "files/leaderboard.txt";
    _rows = r;
    _cols = c;

    _height = (_rows*16)+50;
    _width = _cols*16;

    init_fonts();
    init_leaderboard_scores();
    init_title();

}

void Leaderboard_Window::init_fonts(){
    if (! _text_font.loadFromFile("files/font.ttf")){
        std::cout << "error loading font\n";
    }
}

void Leaderboard_Window::init_window(){

    render_window.create(sf::VideoMode(_width, _height), "leaderboard", sf::Style::Close);
}

void Leaderboard_Window::init_leaderboard_scores(){
    // scan in scores vector from leaderboard.txt
    // min:sec, user_name
    read_leaderboard();

    set_leaderboard_text();
}

void Leaderboard_Window::init_title(){
    // sets up text!
    sf::Vector2f title_pos(_width/2, (_height/2) - 120);

}

void Leaderboard_Window::set_leaderboard_text(){
    // setup leaderboard text based on leaderboard scores

    int i = 0;

    std::string leaderboard_string = "";

    // todo make a 1. --- 2. ---- as well

    while(i < 5 && i < _scores_leaderboard.size()){
        
        leaderboard_string += std::to_string(i+1) + ".\t";
        leaderboard_string += _scores_leaderboard[i].get_time_str() + "\t";
        leaderboard_string += _scores_leaderboard[i].name; 
        if (_scores_leaderboard[i].new_score == true){
            leaderboard_string += "*";
        }
        leaderboard_string += "\n\n"; 
        i += 1;
    }

    sf::Vector2f board_pos(_width/2, (_height/2) + 20);
    setup_text(_leaderboard_text, leaderboard_string, _text_font, true, false, sf::Color::White, 18, board_pos);
    
}

// ===== Display Loops =====

void Leaderboard_Window::display_leaderboard(){
    init_window();
    
    while (render_window.isOpen()){
        // draw board, and then poll event
        sf::Event event;

        while(render_window.pollEvent(event)){
            switch (event.type)
            {

            case sf::Event::Closed:
                // close window but first write leaderboard contents to leaderboard.txt
                // write_leaderboard();
                render_window.close();
                write_leaderboard();
                
                return;

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

    return;

}

bool scores_are_same(Score score1, Score score2){
    if (score1.name.compare(score2.name)){
        return false;
    }

    if(score1.min != score2.min){
        return false;
    }

    if (score1.sec != score2.sec){
        return false;
    }

    return true;
}

bool Leaderboard_Window::has_score(std::string username, int min, int sec){
    // boolean for if score is scored already in vector
    Score temp_score(username, min, sec);

    for(int i = 0; i < _scores_leaderboard.size(); i += 1){
        if (scores_are_same(temp_score, _scores_leaderboard[i])){
            
            // if score are the same, leaderboard already has this score
            return true;
        }
    }
    
    // we went through all the scores, they are not the same
    return false;
}

// ====== Drawing ======

void Leaderboard_Window::draw_all(){
    render_window.draw(_title);
    render_window.draw(_leaderboard_text);
}

// ====== Helpers ======

void Leaderboard_Window::setup_text(sf::Text &text, std::string message, sf::Font &font, bool bold, bool underlined, sf::Color color, int size, sf::Vector2f position){
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

void Leaderboard_Window::setTextPos(sf::Text &text, float x, float y){
    // set the text position of text
    sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin(textRect.left + textRect.width/2.0f,
                    textRect.top+textRect.height/2.0f);
    text.setPosition(sf::Vector2f(x,y));
}

void Leaderboard_Window::read_leaderboard(){
    // read in from csv leaderboard.txt
    std::ifstream file(_leaderboard_filename);
    if (!file.is_open()){
        std::cout << "Error reading file!\n";
    }

    std::string line;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        // Split the line into tokens using a comma as the delimiter
        while (std::getline(ss, token, ',')) {
            // Remove leading and trailing whitespaces from the token
            tokens.push_back(token);
        }

        std::string time_string = tokens[0];
        std::string name = tokens[1];
        int min = std::stoi(time_string.substr(0,2));
        int sec = std::stoi(time_string.substr(3, 5));
        Score cur_score(name, min, sec);

        _scores_leaderboard.push_back(cur_score);
    
    }

    // Close the file
    file.close();

    sort_scores();

}

void Leaderboard_Window::write_leaderboard(){

    std::ofstream out_file(_leaderboard_filename);


    if(!out_file.is_open()){
        std::cout << "Failed to open!" << std::endl;
    }

    for(int i = 0; i < _scores_leaderboard.size(); i += 1){
        
        std::string cur_line = _scores_leaderboard[i].get_line_string();
        out_file << cur_line;
    }

    out_file.close();

}

void Leaderboard_Window::print_scores(){
    for (int i = 0; i < _scores_leaderboard.size(); i += 1){
        _scores_leaderboard[i].print();
    }
}

void Leaderboard_Window::sort_scores(){
    // Sort the vector using a lambda function as the custom comparator
    std::sort(_scores_leaderboard.begin(), _scores_leaderboard.end(), [](Score a, Score b) {
        // bool if a < b 
        int seconds_a = a.min*60 + a.sec;
        int seconds_b = b.min*60 + b.sec;

        return seconds_a < seconds_b;

    });

}

void Leaderboard_Window::add_score(std::string name, int min, int sec){
    // adds user to leaderboard (make sure theyre marked as new)

    Score new_score(name, min, sec, true);

    _scores_leaderboard.push_back(new_score);

    sort_scores();

    set_leaderboard_text();
}

