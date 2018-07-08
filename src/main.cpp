/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include "GameConfig.h"
#include <string>
#include "tinyxml2.h"
#include <memory>
#include <map>

//Screen dimension constants  
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

#define SPRITE_COUNT 10000

#define LOG(...)  printf("[%s:%d] ", __FILE__, __LINE__); printf(__VA_ARGS__)

SDL_Rect rects[SPRITE_COUNT];

using namespace std;
using namespace tinyxml2;
class Glyph;

typedef map<string, shared_ptr<Glyph> > StringToSharedGlyphPtrMap;
typedef pair<string, shared_ptr<Glyph> > StringToSharedGlyphPtrPair;

class Glyph {
private:
	int offsetX;
	int offsetY;
	int advance;
	std::shared_ptr<SDL_Rect> rect;
	shared_ptr<string> value;

public: 
	Glyph(int offsetX, int offsetY, int advance,
		std::shared_ptr<SDL_Rect> rect, shared_ptr<string> value) {
		this->offsetX = offsetX;
		this->offsetY = offsetY;
		this->advance = advance;
		this->rect = rect;
		this->value = value;
	}

	shared_ptr<SDL_Rect> getRect() {
		return rect;
	}

	int getOffsetX() {
		return offsetX;
	}

	int getOffsetY() {
		return offsetY;
	}

	int getAdvance() {
		return advance;
	}
};

class Font {
private:
	unique_ptr<StringToSharedGlyphPtrMap> glyphMap;
	SDL_Texture* texture;
	int height;

public:
	Font(unique_ptr<StringToSharedGlyphPtrMap> glyphMap, SDL_Texture* texture, int height) {
		this->glyphMap = std::move(glyphMap);
		this->texture = texture;
		this->height = height;
	}

	~Font() {
		SDL_DestroyTexture(texture);
	}

	void render(SDL_Renderer* renderer, const string& text, int x, int y) {
		y += height / 2;
		for (int i = 0; i < text.size(); i++) {
			string key = string(1, text.at(i));
			shared_ptr<Glyph> glyph = glyphMap->at(key);
			
			shared_ptr<SDL_Rect> source = glyph->getRect();
			SDL_Rect destination = { x + glyph->getOffsetX(), y - glyph->getOffsetY(), source->w, source->h };
			
			SDL_RenderCopy(renderer, texture, source.get(), &destination);
			
			x += glyph->getAdvance();
		}
	}
};

SDL_Texture* loadTexture(string path, SDL_Renderer* renderer, int* success) {
	LOG("Loading texture: %s\n", path.c_str());
	*success = 1;
	SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
	if (texture == NULL)
	{
		LOG("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
		return NULL;
	}

	*success = 0;
	return texture;
}

unique_ptr<Font> loadFont(string path, SDL_Renderer* renderer, int* failure) {
	*failure = 1;

	LOG("Loading Font: %s\n", path.c_str());
	string texPath = path;
	texPath += ".png";

	int fail;
	SDL_Texture* texture = loadTexture(texPath, renderer, &fail);
	if (fail) {
		return unique_ptr<Font>(NULL);
	}

	path += ".xml";
	XMLDocument doc;
	doc.LoadFile(path.c_str());
	XMLNode* fontNode = doc.FirstChildElement("font");
	if (!fontNode) {
		return unique_ptr<Font>(NULL);
	}
	int height;
	fontNode->FirstChildElement("metrics")->QueryIntAttribute("height", &height);


	XMLNode* charNode = 
		fontNode->FirstChildElement("chars")->FirstChild();

	XMLElement* element;

	unique_ptr<StringToSharedGlyphPtrMap> valueToGlyphMap = unique_ptr<StringToSharedGlyphPtrMap>(new StringToSharedGlyphPtrMap());
	const char* tmpValue = "Temp Value";
	StringToSharedGlyphPtrPair pair;
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
		element->QueryStringAttribute("id", &tmpValue);

		string value = string(tmpValue);

		SDL_Rect* rect = new SDL_Rect;
		rect->x = rectX;
		rect->y = rectY;
		rect->w = rectW;
		rect->h = rectH;

		std::shared_ptr <Glyph> glyph = shared_ptr<Glyph>(new Glyph(offsetX, offsetY, advance,
			shared_ptr<SDL_Rect>(rect),
			shared_ptr<string>(new string(value.c_str()))));

		pair.first = value;
		pair.second = glyph;

		valueToGlyphMap->insert(pair);

		charNode = charNode->NextSibling();
	}
	
	*failure = 0;
	return unique_ptr<Font>(new Font(std::move(valueToGlyphMap), texture, height));
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
               		SDL_SetRenderDrawColor( gRenderer, 0x00, 0x00, 0x00, 0xFF ); 

				    //Load image at specified path
					int failure;
					gTexture = loadTexture(textureName, gRenderer, &failure);
					if (failure) {
						LOG("Failed to load texture\n");
						return 1;
					}
					Uint32 format;
					int access;
					SDL_QueryTexture(gTexture, &format, &access, &texWidth, &texHeight);

					LOG("Texture Loaded: %s: Format(%s) Access(%d) Width(%d) Height(%d)\n",
						textureName.c_str(), SDL_GetPixelFormatName(format), access, texWidth, texHeight);

				    for (int i = 0; i < SPRITE_COUNT; i++) {
				    	rects[i].x = rand() % SCREEN_WIDTH;
				    	rects[i].y = rand() % SCREEN_HEIGHT+50;
				    	rects[i].w = 40;
				    	rects[i].h = 40;
				    }
					unique_ptr<Font> font = loadFont("../assets/arial_regular_10", gRenderer, &failure);
					if (failure) {
						LOG("Failed to load font\n");
						return 1;
					}

					//Main loop flag
					bool quit = false;

					//Event handler
					SDL_Event e;

					Uint64 start;
					Uint64 end;
					Uint32 lastLoggedFPS = 0;

					char fpsBuf[20];
					string fpsText = "";

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

						font->render(gRenderer, fpsText, 10, 10);
		                //Update screen
		                SDL_RenderPresent( gRenderer );

		                end = SDL_GetPerformanceCounter();

		                float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
		                Uint32 now = SDL_GetTicks();
		                if (now - lastLoggedFPS > 1000) {
							sprintf(fpsBuf, "FPS: %d",  (int)(1.0f / elapsed));
							fpsText = fpsBuf;
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