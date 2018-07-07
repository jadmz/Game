/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include "GameConfig.h"
#include <string>
#include "tinyxml2.h"

//Screen dimension constants  
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

#define SPRITE_COUNT 20000

#define LOG(...)  printf("[%s:%d] ", __FILE__, __LINE__); printf(__VA_ARGS__)

SDL_Rect rects[SPRITE_COUNT];

using namespace std;
using namespace tinyxml2;

class Font {

};

int loadFont(string path, Font* font) {
	LOG("Loading Font: %s\n", path.c_str());
	string newPath = path;
	path += ".xml";
	XMLDocument doc;
	doc.LoadFile(path.c_str());

	XMLNode* charNode = 
		doc.FirstChildElement("font")->FirstChildElement("chars")->FirstChild();

	XMLElement* element;
	while (charNode) {
		element = charNode->ToElement();
		
		int offsetX;
		int offsetY;
		int advance;
		int rectX;
		int rectY;
		int rectW;
		int rectH;
		element->QueryIntAttribute("advance", &advance);
		element->QueryIntAttribute("offset_x", &offsetX);
		element->QueryIntAttribute("offset_y", &offsetY);
		element->QueryIntAttribute("rect_x", &rectX);
		element->QueryIntAttribute("rect_y", &rectY);
		element->QueryIntAttribute("rect_w", &rectW);
		element->QueryIntAttribute("rect_h", &rectH);

		LOG("Advance:%d, OffsetX: %d, OffsetY: %d, RectX: %d, RectY: %d, RectW: %d, RectH: %d\n",
			advance,
			offsetX,
			offsetY,
			rectX,
			rectY,
			rectW,
			rectH);
		charNode = charNode->NextSibling();
	}
	//LOG("%s\n", doc.FirstChild()->NextSibling());

	return 0;
}

int main( int argc, char* args[] )
{
	LOG("%s Version %d.%d\n",
            args[0],
            Game_VERSION_MAJOR,
            Game_VERSION_MINOR);

	//The window we'll be rendering to
	SDL_Window* gWindow = NULL;
	
	//The window renderer
	SDL_Renderer* gRenderer = NULL;

	//Current displayed texture
	SDL_Texture* gTexture = NULL;
	int texWidth;
	int texHeight;

	string textureName = "../assets/test.png";

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		LOG( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
	}
	else
	{
		int imgFlags = IMG_INIT_PNG;
		if( !( IMG_Init( imgFlags ) & imgFlags ) )
		{
			LOG( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
			return 1;
		}
		else
		{
			//Create window
			gWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
			if( gWindow == NULL )
			{
				LOG( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
			}
			else
			{
				 //Create renderer for window
	            gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED );
	            if( gRenderer == NULL )
	            {
	                LOG( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
	                return 1;
	            }
	            else
	            {
					//Initialize renderer color
               		SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xF0, 0x00, 0xFF ); 

				    //Load image at specified path
				    gTexture = IMG_LoadTexture(gRenderer, textureName.c_str() );
				    if( gTexture == NULL )
				    {
				        LOG( "Unable to load image %s! SDL_image Error: %s\n", textureName.c_str(), IMG_GetError() );
				        return 1;
				    }
				    else
				    {
				    	Uint32 format;
				    	int access;
				    	SDL_QueryTexture(gTexture, &format, &access, &texWidth, &texHeight);

				    	LOG("Texture Loaded: %s: Format(%s) Access(%d) Width(%d) Height(%d)\n",
				    		textureName.c_str(), SDL_GetPixelFormatName(format), access, texWidth, texHeight);
				    }

				    for (int i = 0; i < SPRITE_COUNT; i++) {
				    	rects[i].x = rand() % SCREEN_WIDTH;
				    	rects[i].y = rand() % SCREEN_HEIGHT;
				    	rects[i].w = 40;
				    	rects[i].h = 40;
				    }
					Font font;
					loadFont("..\\assets\\arial_regular_20", &font);

					//Main loop flag
					bool quit = false;

					//Event handler
					SDL_Event e;

					Uint64 start;
					Uint64 end;
					Uint32 lastLoggedFPS = 0;

					//While application is running
					while( !quit )
					{
						start = SDL_GetPerformanceCounter();
						//Handle events on queue
						while( SDL_PollEvent( &e ) != 0 )
						{
							//User requests quit
							if( e.type == SDL_QUIT )
							{
								quit = true;
							}
						}

						//Clear screen
		                SDL_RenderClear( gRenderer );

		                for (int i = 0; i < SPRITE_COUNT; i++) {
		                	//Render texture to screen
		                	SDL_RenderCopy( gRenderer, gTexture, NULL, &(rects[i]));
		                }

		                //Update screen
		                SDL_RenderPresent( gRenderer );

		                end = SDL_GetPerformanceCounter();

		                float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
		                Uint32 now = SDL_GetTicks();
		                if (now - lastLoggedFPS > 1000) {
		                	LOG("FPS: %.2f\n", 1.0f/elapsed);
		                	lastLoggedFPS = now;
		                }
					}
				}
			}
		}
	}

	//Destroy window
	SDL_DestroyWindow( gWindow );

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}