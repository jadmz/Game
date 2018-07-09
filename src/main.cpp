/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

// Using SDL and standard IO
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <map>
#include <memory>
#include <string>
#include "GameConfig.h"
#include "fonts.h"
#include "tiles.h"
#include "render.h"
#include "assets.h"
#include "common.h"

#define SPRITE_COUNT 500

SDL_Rect rects[SPRITE_COUNT];


bool initSystem(SDL_Window** window) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    LOG("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  return initRenderingSystem(window);
}

void run(SDL_Window* window) {
  shared_ptr<Renderer> renderer = shared_ptr<Renderer>(NULL);

  if (!createRenderer(window, &renderer)) {
    LOG("Failed to create system! Exiting...\n");
    return;
  }

  unique_ptr<AssetManager> assetManager =
      unique_ptr<AssetManager>(new AssetManager(renderer));

  // Current displayed texture
  shared_ptr<Texture> texture = shared_ptr<Texture>(NULL);

  string textureName = "../assets/test.png";

  if (!assetManager->getTexture(textureName, &texture)) {
    LOG("Failed to load texture! Exiting...\n");
    return;
  }

  for (int i = 0; i < SPRITE_COUNT; i++) {
    rects[i].x = rand() % SCREEN_WIDTH;
    rects[i].y = rand() % SCREEN_HEIGHT + 50;
    rects[i].w = 40;
    rects[i].h = 40;
  }

  int failure;
  shared_ptr<Font> font;
  if (!assetManager->getFont("../assets/arial_regular_10", &font)) {
    LOG("Failed to load font\n");
    return;
  }

  shared_ptr<TilePalette> tilePalette;
  if (!assetManager->getTilePalette("test-path", tilePalette)) {
  	LOG("Error loading tile palette!\n");
  	return;
  }

  shared_ptr<TileInstance> tileInstance;

  if (!tilePalette->createTileInstance("white-block", tileInstance)) {
  	LOG("failed to create tile instance\n");
  	return;
  }

  // Main loop flag
  bool quit = false;

  // Event handler
  SDL_Event e;

  Uint64 start;
  Uint64 end;
  Uint32 lastLoggedFPS = 0;

  char fpsBuf[20];
  string fpsText = "";

  // While application is running
  while (!quit) {
    start = SDL_GetPerformanceCounter();
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0) {
      // User requests quit
      if (e.type == SDL_QUIT) {
        quit = true;
      }
    }

    // Clear screen
    renderer->clear();

    // for (int i = 0; i < SPRITE_COUNT; i++) {
    //   // Render texture to screen
    //   renderer->render(*texture, NULL, &(rects[i]));
    // }

		SDL_Rect rect{100, 100, 32, 32}	;

    tileInstance->render(*renderer, rect);


    font->render(*renderer, fpsText, 10, 10);
    // Update screen

    renderer->present();

    end = SDL_GetPerformanceCounter();

    float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
    Uint32 now = SDL_GetTicks();
    if (now - lastLoggedFPS > 1000) {
      sprintf(fpsBuf, "FPS: %d", (int)(1.0f / elapsed));
      fpsText = fpsBuf;
      lastLoggedFPS = now;
    }
  }
}

int main(int argc, char* args[]) {
  LOG("%s Version %d.%d\n", args[0], Game_VERSION_MAJOR, Game_VERSION_MINOR);

  LOG("SDL Compiled Version: %d\n", SDL_COMPILEDVERSION);
  // The window we'll be rendering to
  SDL_Window* window = NULL;

  // Initialize SDL
  if (!initSystem(&window)) {
    LOG("Failed to initialize system! Exiting...\n");
    return 1;
  }

  run(window);

  // Destroy window
  SDL_DestroyWindow(window);

  // Quit SDL subsystems
  SDL_Quit();

  return 0;
}