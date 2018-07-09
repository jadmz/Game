#ifndef ASSETS_INCLUDED
#define ASSETS_INCLUDED

#include <memory>
#include "common.h"
#include "fonts.h"
#include "render.h"

using namespace std;
class Font;
class AssetManager;
class TilePalette;

bool loadFont(string, AssetManager*, shared_ptr<Font>*);
bool loadTilePalette(string, AssetManager&, shared_ptr<TilePalette>&);


typedef map<string, shared_ptr<Texture> > PathToTextureMap;
typedef map<string, shared_ptr<Font> > PathToFontMap;
typedef map<string, shared_ptr<TilePalette> > PathToTilePaletteMap;

class AssetManager {
 private:
  unique_ptr<PathToTextureMap> textures;
  unique_ptr<PathToFontMap> fonts;
  unique_ptr<PathToTilePaletteMap> tilePalettes;

  shared_ptr<Renderer> renderer;

 public:
  AssetManager(shared_ptr<Renderer> renderer) {
    textures = unique_ptr<PathToTextureMap>(new PathToTextureMap());
    fonts = unique_ptr<PathToFontMap>(new PathToFontMap());
    tilePalettes = unique_ptr<PathToTilePaletteMap>(new PathToTilePaletteMap());

    this->renderer = renderer;
  }

  bool getTexture(const string path, shared_ptr<Texture>* texture) {
    PathToTextureMap::iterator it = textures->find(path);
    if (it == textures->end()) {
      if (!renderer->loadTexture(path, texture)) {
        LOG("Error loading texture: %s\n", path.c_str());
        return false;
      }

      (*textures)[path] = *texture;
      return true;
    }

    *texture = it->second;
    return true;
  }

  bool getFont(const string path, shared_ptr<Font>* font) {
    PathToFontMap::iterator it = fonts->find(path);
    if (it == fonts->end()) {
      if (!loadFont(path, this, font)) {
        LOG("Error loading font: %s\n", path.c_str());
        return false;
      }

      (*fonts)[path] = *font;
      return true;
    }

    *font = it->second;
    return true;
  }

  bool getTilePalette(const string path, shared_ptr<TilePalette>& tilePalette) {
    PathToTilePaletteMap::iterator it = tilePalettes->find(path);
    if (it == tilePalettes->end()) {
      if (!loadTilePalette(path, *this, tilePalette)) {
        LOG("Error loading tilePalette: %s\n", path.c_str());
        return false;
      }

      (*tilePalettes)[path] = tilePalette;
      return true;
    }

    tilePalette = it->second;
    return true;
  }
};

#endif