#ifndef TILES_INCLUDED
#define TILES_INCLUDED

#include <SDL.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common.h"

using namespace std;

class TileDefinition;
typedef map<string, shared_ptr<TileDefinition> > NameToTileDefinitionMap;

class TileDefinition {
 private:
  string name;
  unique_ptr<SDL_Rect> texRect;

 public:
  TileDefinition(string name, unique_ptr<SDL_Rect> texRect) {
    this->name = name;
    this->texRect = move(texRect);
  }

  string getName() const { return name; }

  SDL_Rect* getTexRect() const {
    return texRect.get();
  }
};

class TileInstance {
 private:
  shared_ptr<TileDefinition> tileDefinition;
  shared_ptr<Texture> texture;

 public:
  TileInstance(shared_ptr<TileDefinition> tileDefinition, shared_ptr<Texture> texture) {
    this->tileDefinition = tileDefinition;
    this->texture = texture;
  }

  void render(const Renderer& renderer, SDL_Rect& dest) {
    renderer.render(*texture, tileDefinition->getTexRect(), &dest);
  }
};

class TilePalette {
 private:
  shared_ptr<Texture> texture;
  unique_ptr<NameToTileDefinitionMap> tileDefinitions;

 public:
  TilePalette(shared_ptr<Texture> texture,
              const vector<shared_ptr<TileDefinition> >& tileDefinitions) {
    this->texture = texture;
    this->tileDefinitions =
        unique_ptr<NameToTileDefinitionMap>(new NameToTileDefinitionMap());

    for (const shared_ptr<TileDefinition> tile : tileDefinitions) {
      (*(this->tileDefinitions))[tile->getName()] = tile;
    }
  }

  bool createTileInstance(string name, shared_ptr<TileInstance>& tileInstance) {
    NameToTileDefinitionMap::iterator it = tileDefinitions->find(name);
    if (it == tileDefinitions->end()) {
      LOG("No tile associated with name: %s\n", name.c_str());
      return false;
    }

    tileInstance = shared_ptr<TileInstance>(new TileInstance(it->second, texture));

    return true;
  }
};

bool loadTilePalette(string path, AssetManager& assetManager, shared_ptr<TilePalette>& tilePalette) {
  shared_ptr<Texture> paletteTexture;

  if (!assetManager.getTexture("../assets/test-block.png", &paletteTexture)) {
    LOG ("Error loading tile palette: %s\n", path.c_str());
    return false;
  }

  vector<shared_ptr<TileDefinition>> tileDefinitions = vector<shared_ptr<TileDefinition>>();

  SDL_Rect* rect = new SDL_Rect;
  rect->x = 0;
  rect->y = 0;
  rect->w = paletteTexture->getWidth();
  rect->h = paletteTexture->getHeight();

  tileDefinitions.push_back(shared_ptr<TileDefinition>(new TileDefinition("white-block", unique_ptr<SDL_Rect>(rect))));

  tilePalette = shared_ptr<TilePalette>(new TilePalette(paletteTexture, tileDefinitions));

  return true;
}


#endif