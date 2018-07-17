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
class TileInstance;

typedef map<string, shared_ptr<TileDefinition> > NameToTileDefinitionMap;
typedef map<int, shared_ptr<TileInstance> > PosToTileInstanceMap;


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

  void render(const Renderer& renderer, int x , int y) {
    SDL_Rect* source = tileDefinition->getTexRect();
    SDL_Rect dest {x, y, source->w, source->h};
    renderer.render(*texture, source, &dest);
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

class TileMap {
private:
  unique_ptr<PosToTileInstanceMap> tileInstances;
  int mapWidth;
  int mapHeight;
  int tileLength;

  int convertToKey(int x, int y) {
    return y * mapWidth + x;
  }

  void convertKeyToXY(int key, int& x, int& y) {
    x = key % mapWidth;
    y = key / mapWidth;
  }

public:
  TileMap(int mapWidth, int mapHeight, int tileLength) {
    this->mapWidth = mapWidth;
    this->mapHeight = mapHeight;
    this->tileLength = tileLength;
    tileInstances = unique_ptr<PosToTileInstanceMap>(new PosToTileInstanceMap());
  }

  int getMapWidth() const {
    return mapWidth;
  }

  int getMapHeight() const {
    return mapHeight;
  }

  void set(int x, int y, shared_ptr<TileInstance> tileInstance) {
    (*tileInstances)[convertToKey(x,y)] = tileInstance;
  }

  void render(Renderer& renderer, int originX, int originY) {
    for (auto const& keyAndTileInstance : *tileInstances)
    {
        int x;
        int y;
        convertKeyToXY(keyAndTileInstance.first, x, y);

        keyAndTileInstance.second->render(renderer, x * tileLength, y * tileLength);
    }
  }
};

bool loadTilePalette(string path, AssetManager& assetManager, shared_ptr<TilePalette>& tilePalette) {
  shared_ptr<Texture> paletteTexture;

  if (!assetManager.getTexture("../assets/gray-block.png", &paletteTexture)) {
    LOG ("Error loading tile palette: %s\n", path.c_str());
    return false;
  }

  vector<shared_ptr<TileDefinition>> tileDefinitions = vector<shared_ptr<TileDefinition>>();

  SDL_Rect* rect = new SDL_Rect;
  rect->x = 0;
  rect->y = 0;
  rect->w = paletteTexture->getWidth();
  rect->h = paletteTexture->getHeight();

  tileDefinitions.push_back(shared_ptr<TileDefinition>(new TileDefinition("gray-block", unique_ptr<SDL_Rect>(rect))));

  tilePalette = shared_ptr<TilePalette>(new TilePalette(paletteTexture, tileDefinitions));

  return true;
}


#endif