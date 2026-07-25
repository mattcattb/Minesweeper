#include "ui/texture_manager.hpp"
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include <string>

namespace minesweeper::ui {

std::unordered_map<std::string, sf::Texture> TextureManager::textures;

sf::Texture& TextureManager::getTexture(std::string textureName){
    auto result = textures.find(textureName);
    if (result == textures.end()){
        // texture does not already exist in the map, go get it
        sf::Texture newTexture;
        newTexture.loadFromFile("assets/images/" + textureName + ".png");
        textures[textureName] = newTexture;
        return textures[textureName];
    }else{
        return result->second;
    }
}

}  // namespace minesweeper::ui
