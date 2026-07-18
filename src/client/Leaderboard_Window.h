#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>


struct Score{

    std::string name;
    int min;
    int sec;
    bool new_score;

    Score(std::string n, int m, int s, bool ns = false){
        name = n;
        min = m;
        sec = s;

        // store if value was added from game (use to add star)
        new_score = ns;
    }
    
    std::string get_time_str(){
        // returns seconds and min as a min:sec string
        std::string time_str = "";
        if (min < 10){
            time_str += "0";
        }
        time_str += std::to_string(min) + ":";

        if(sec < 10){
            time_str += "0"; 
        }

        time_str += std::to_string(sec);

        return time_str;
    }

    std::string get_line_string(){
        // get all the data into a full line
        // min:sec,name
        std::string ans = get_time_str() + "," + name + "\n";
        return ans;
    }

    void print(){
        std::cout << "\nName: " << name << std::endl;
        std::cout << get_time_str() << std::endl;
    }
};

class Leaderboard_Window{

    // Variables
    std::string _leaderboard_filename;
    std::vector<Score> _scores_leaderboard;

    int _rows;
    int _cols;
    int _width, _height;

    // SFML Variables
    sf::Font _text_font;
    sf::Text _title;
    sf::Text _leaderboard_text;
    sf::RenderWindow render_window;

    // init welcome window objects
    void init_fonts();
    void init_window();
    void init_leaderboard_scores();
    void init_title();
    
    void set_leaderboard_text();

    // drawing
    void draw_all();

    // helpers
    void setup_text(sf::Text &text, std::string message, sf::Font &font, bool bold, bool underlined, sf::Color color, int size, sf::Vector2f position);
    void setTextPos(sf::Text &text, float x, float y);
    void sort_scores();

    // Read and Write
    void read_leaderboard();
    void write_leaderboard();
    void print_scores();

public:


    bool has_score(std::string username, int min, int sec);
    Leaderboard_Window(int r, int c);
    void display_leaderboard();
    void add_score(std::string name, int min, int sec); // adds user to leaderboard

};

