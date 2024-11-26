#pragma once
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include <string>


class Texture_Manager{
    static std::unordered_map<std::string, sf::Texture> textures;

public:
    
    static sf::Texture& getTexture(std::string textureName);
};