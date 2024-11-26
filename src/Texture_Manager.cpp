#include "Texture_Manager.h"
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include <string>

std::unordered_map<std::string, sf::Texture> Texture_Manager::textures;

sf::Texture& Texture_Manager::getTexture(std::string textureName){
    auto result = textures.find(textureName);
    if (result == textures.end()){
        // texture does not already exist in the map, go get it
        sf::Texture newTexture;
        newTexture.loadFromFile("files/images/" + textureName + ".png");
        textures[textureName] = newTexture;
        return textures[textureName];
    }else{
        return result->second;
    }
}