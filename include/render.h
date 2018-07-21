#ifndef RENDER_INCLUDED
#define RENDER_INCLUDED

#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <memory>
#include "common.h"
#include "Box2D/Box2D.h"

using namespace std;


class Texture {
private:
	SDL_Texture* texture;
	int width;
	int height;

public:
	Texture(SDL_Texture* texture) {
		this->texture = texture;

		Uint32 format;
		int access;
		SDL_QueryTexture(texture, &format, &access, &width, &height);
	}

	~Texture() {
		SDL_DestroyTexture(texture);
	}

	SDL_Texture* getSDLTexture() const {
		return texture;
	}

  int getWidth() const {
    return width;
  }

  int getHeight() const {
    return height;
  }
};

class Renderer : public b2Draw {

private:
	SDL_Renderer* renderer;

public:
	Renderer(SDL_Renderer* renderer) {
		this->renderer = renderer;
	}

	~Renderer() {
		SDL_DestroyRenderer(renderer);
	}

	void clear() const {
    SDL_SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0xFF ); 

		SDL_RenderClear( renderer );
	}

	void render(const Texture& tex, SDL_Rect* source, SDL_Rect* dest) const {
		SDL_RenderCopy(renderer, tex.getSDLTexture(), source, dest);	
	}

	void present() const {
		SDL_RenderPresent( renderer );
	}

	bool loadTexture(const string path, shared_ptr<Texture>* texture) const {
		LOG("Loading texture: %s\n", path.c_str());
		
		SDL_Texture* sdlTex = IMG_LoadTexture(renderer, path.c_str());
		if (sdlTex == NULL)
		{
			LOG("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
			return false;
		}

		*texture = shared_ptr<Texture>(new Texture(sdlTex));

		return true;
	}

  /// Draw a closed polygon provided in CCW order.
  void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
    if (vertexCount != 4) {
      LOG("Polygon is not a square!!\n");
    }
    SDL_SetRenderDrawColor(renderer, color.r * 255, color.g * 255, color.b* 255, color.a * 255);

    float minX = 10000.0f;
    float maxX = -10000.0f;
    float minY = 10000.0f;
    float maxY = -10000.0f;

    for(int i = 0; i < vertexCount; i++) {
      if (vertices[i].x < minX) {
        minX = vertices[i].x;
      }
      if (vertices[i].x > maxX) {
        maxX = vertices[i].x;
      }

      if (-vertices[i].y < minY) {
        minY = -vertices[i].y;
      }

      if (-vertices[i].y > maxY) {
        maxY = -vertices[i].y;
      }
    } 


    SDL_Rect rect {
      (int) (minX * B2_UNITS_TO_PIXELS),
      (int) (minY * B2_UNITS_TO_PIXELS),
      (int) ((maxX - minX) * B2_UNITS_TO_PIXELS),
      (int) ((maxY - minY) * B2_UNITS_TO_PIXELS)
    };

    SDL_RenderDrawRect(renderer, &rect);
  }

  /// Draw a solid closed polygon provided in CCW order.
  void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
    if (vertexCount != 4) {
      LOG("Polygon is not a square!!\n");
    }
    SDL_SetRenderDrawColor(renderer, color.r * 255, color.g * 255, color.b* 255, color.a * 255);

    float minX = 10000.0f;
    float maxX = -10000.0f;
    float minY = 10000.0f;
    float maxY = -10000.0f;

    for(int i = 0; i < vertexCount; i++) {
      if (vertices[i].x < minX) {
        minX = vertices[i].x;
      }
      if (vertices[i].x > maxX) {
        maxX = vertices[i].x;
      }

      if (-vertices[i].y < minY) {
        minY = -vertices[i].y;
      }

      if (-vertices[i].y > maxY) {
        maxY = -vertices[i].y;
      }
    } 


    SDL_Rect rect {
      (int) (minX * B2_UNITS_TO_PIXELS),
      (int) (minY * B2_UNITS_TO_PIXELS),
      (int) ((maxX - minX) * B2_UNITS_TO_PIXELS),
      (int) ((maxY - minY) * B2_UNITS_TO_PIXELS)
    };

    SDL_RenderFillRect(renderer, &rect);
  }

  /// Draw a circle.
  void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) {

  }
  
  /// Draw a solid circle.
  void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) {

  }
  
  /// Draw a line segment.
  void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {

  }

  /// Draw a transform. Choose your own length scale.
  /// @param xf a transform.
  void DrawTransform(const b2Transform& xf) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect {
      (int) (xf.p.x * B2_UNITS_TO_PIXELS)-3,
      (int) -(xf.p.y * B2_UNITS_TO_PIXELS) -3,
      6,
      6
    };

    SDL_RenderFillRect(renderer, &rect);
  }
};


class Sprite {
private:
	shared_ptr<Texture> texture;

public:
	Sprite(shared_ptr<Texture> texture) {
		this->texture = texture;
	}

	void render(const Renderer& renderer, SDL_Rect* dest) {
		renderer.render(*texture, NULL, dest);
	}
};

bool initRenderingSystem() {
	int imgFlags = IMG_INIT_PNG;
	if( !( IMG_Init( imgFlags ) & imgFlags ) )
	{
		LOG( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
		return false;
	}
  return true;
}

bool createWindow(SDL_Window** window) {
  //Create window
  *window = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
  if( *window == NULL )
  {
    LOG( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
    return false;
  }

  return true;
}

bool createRenderer(SDL_Window* window, shared_ptr<Renderer>* renderer) {
	//Create renderer for window
    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );
    if( sdlRenderer == NULL )
    {
        LOG( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
        return false;
    }

	//Initialize renderer color
	SDL_SetRenderDrawColor( sdlRenderer, 0x00, 0x00, 0x00, 0xFF ); 

	*renderer = shared_ptr<Renderer>(new Renderer(sdlRenderer));

	return true;
}

#endif