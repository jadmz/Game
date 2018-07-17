/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

// Using SDL and standard IO
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <map>
#include <memory>
#include <string>
#include <iostream>
#include "GameConfig.h"
#include "fonts.h"
#include "tiles.h"
#include "render.h"
#include "assets.h"
#include "common.h"
#include <stack>
#include <utility>
#include <cctype>

#define SPRITE_COUNT 500

const int targetFrameDuration = 1000 / 200;  // 200 frames per sec

SDL_Rect rects[SPRITE_COUNT];

class AABB {
private:
	float width;
	float height;

public:
	AABB(float width, float height) {
		this->width = width;
		this->height = height;
	}
};

class Vector {
 private:

 public:
 	float x;
 	float y;

  Vector() : Vector(0.0f, 0.0f) {}

  Vector (float x, float y) {
  	this->x = x;
  	this->y = y;
  }

  float getX() {
  	return x;
  }

  float getY() {
  	return y;
  }
};

class Transform {
 private:
  Vector position;
  float rotation;
  float scale;

 public:
  Transform() {
    position = Vector();
    rotation = 0.0f;
    scale = 1.0f;
  }

  Vector getPosition() const { return position; }

  void setPosition(Vector position) { this->position = position; }

  float getRotation() const { return rotation; }

  void setRotation(float rotation) { this->rotation = rotation; }

  float getScale() const { return scale; }

  void setScale(float scale) { this->scale = scale; }
};

class Body {
 private:
  shared_ptr<Transform> transform;
  AABB aabb;
  Vector velocity;

 public:
  Body(float width, float height) : aabb(width, height) {
    transform = shared_ptr<Transform>(new Transform());
    velocity = Vector();
  }

  shared_ptr<Transform> getTransform() const { return transform; }

  AABB getAabb() const { 
  	return aabb;
  }

  Vector getVelocity() const {return velocity; }

  void setVelocity(Vector velocity) {
  	this->velocity = velocity;
  }
};

bool initSystem() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    LOG("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  return initRenderingSystem();
}

void updatePhysics(TileMap& tileMap, Body& body) {
	Vector pos = body.getTransform()->getPosition();
	Vector velocity = body.getVelocity();

	body.getTransform()->setPosition(Vector(pos.x + velocity.x, pos.y + velocity.y));
}

void run() {
	SDL_Window* window;

	if (!createWindow(&window)) {
		LOG("Failed to create window!\n");
		return;
	}

  shared_ptr<Renderer> renderer = shared_ptr<Renderer>(NULL);

  if (!createRenderer(window, &renderer)) {
    LOG("Failed to create system! Exiting...\n");
    return;
  }

  unique_ptr<AssetManager> assetManager =
      unique_ptr<AssetManager>(new AssetManager(renderer));

  // Current displayed texture
  shared_ptr<Texture> texture = shared_ptr<Texture>(NULL);

  string textureName = "../assets/Sprite2.png";

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

  if (!tilePalette->createTileInstance("gray-block", tileInstance)) {
    LOG("failed to create tile instance\n");
    return;
  }

  TileMap tileMap = TileMap(20, 20, 32);

  tileMap.set(4, 4, tileInstance);
  tileMap.set(4, 5, tileInstance);
  tileMap.set(4, 6, tileInstance);
  tileMap.set(5, 6, tileInstance);
  tileMap.set(6, 6, tileInstance);
  tileMap.set(7, 6, tileInstance);

  tileMap.set(9, 6, tileInstance);
  tileMap.set(10, 6, tileInstance);
  tileMap.set(11, 6, tileInstance);
  tileMap.set(12, 6, tileInstance);
  tileMap.set(12, 5, tileInstance);
  tileMap.set(12, 4, tileInstance);
  tileMap.set(13, 4, tileInstance);
  tileMap.set(14, 4, tileInstance);

  SDL_Rect playerDestination{0, 0, texture->getWidth(), texture->getHeight()};
  float playerSpeed = 200.0f / 1000;  // per ms
  float playerX = 20.0f;
  float playerY = 20.0f;

  Body playerBody = Body(32.0f, 32.0f);
  playerBody.getTransform()->setPosition(Vector(20.0f, 20.0f));

  // Main loop flag
  bool quit = false;

  // Event handler
  SDL_Event e;

  Uint64 start;
  Uint64 end;
  Uint32 lastLoggedFPS = 0;

  char fpsBuf[20];
  string fpsText = "";
  int previousFrameStart = SDL_GetTicks();

  // While application is running
  while (!quit) {
    start = SDL_GetPerformanceCounter();
    int frameStart = SDL_GetTicks();
    int dt = frameStart - previousFrameStart;
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0) {
      // User requests quit
      if (e.type == SDL_QUIT) {
        quit = true;
      }
    }

    const Uint8* state = SDL_GetKeyboardState(NULL);
    Vector velocity;
    if (state[SDL_SCANCODE_UP]) {
      velocity.y -= playerSpeed * dt;
    }

    if (state[SDL_SCANCODE_DOWN]) {
      velocity.y += playerSpeed * dt;
    }

    if (state[SDL_SCANCODE_RIGHT]) {
      velocity.x += playerSpeed * dt;
    }

    if (state[SDL_SCANCODE_LEFT]) {
      velocity.x -= playerSpeed * dt;
    }

    playerBody.setVelocity(velocity);

    // LOG ("Player dest: %d, %d\n", playerDestination.x, playerDestination.y);

    updatePhysics(tileMap, playerBody);

    playerDestination.x = playerBody.getTransform()->getPosition().x;
    playerDestination.y = playerBody.getTransform()->getPosition().y;

    // Clear screen
    renderer->clear();

    tileMap.render(*renderer, 0, 0);

    renderer->render(*texture, NULL, &playerDestination);

    font->render(*renderer, fpsText, 10, 10);

    renderer->present();

    end = SDL_GetPerformanceCounter();

    float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
    if (frameStart - lastLoggedFPS > 1000) {
      sprintf(fpsBuf, "FPS: %d", (int)(1.0f / elapsed));
      fpsText = fpsBuf;
      lastLoggedFPS = frameStart;
    }

    previousFrameStart = frameStart;
  }

    // Destroy window
  SDL_DestroyWindow(window);
}

enum TokenType {
	LIST_OPEN = 0,
	LIST_CLOSE,
	IDENTIFIER,
	STRING
};

enum LexVal {
	OPEN_PAREN = 0,
	CLOSE_PAREN,
	SPACE,
	ALPHA_NUMERIC,
	QUOTE
};

class Token {
public:
	TokenType type;
	string value;

	int toString(char* buf) const {
		return sprintf(buf, "TokenType: %d, Value: %s", (int) type, value.c_str());
	}
};

LexVal lex(char c) {
	if (isspace(c)) {
		return SPACE;
	}

	switch (c) {
	  case '(':
	  	return OPEN_PAREN;
	    break;
	  case ')':
		  return CLOSE_PAREN;
	    break;
    case '"':
    	return QUOTE;
    	break;
	  default:
	  	return ALPHA_NUMERIC;
	    break;
  }
}

bool tokenize(string line, vector<Token>& tokens) {
  int i = 0;

  while (i < line.length()) {
    Token token;
    char c = line[i];

    LexVal type = lex(c);
    switch (type) {
      case SPACE: {
        i++;
        break;
      }
      case OPEN_PAREN: {
        token.value = string(1, c);
        token.type = LIST_OPEN;
        tokens.push_back(token);
        i++;
        break;
      }
      case CLOSE_PAREN: {
        token.value = string(1, c);
        token.type = LIST_CLOSE;
        tokens.push_back(token);
        i++;
        break;
      }
      case QUOTE: {
        int quoteEnd;
        bool foundEndQuote = false;

        for (quoteEnd = i + 1; quoteEnd < line.length(); quoteEnd++) {
          if (lex(line[quoteEnd]) == QUOTE) {
            foundEndQuote = true;
            break;
          }
        }

        if (!foundEndQuote) {
          LOG("Missing end quote!\n");
          return false;
        }

        token.value = line.substr(i, quoteEnd - i + 1);
        token.type = STRING;
        tokens.push_back(token);
        i = quoteEnd + 1;
        break;
      }
      case ALPHA_NUMERIC: {
        int end;
        for (end = i + 1;
             end < line.length() && lex(line[end]) == ALPHA_NUMERIC; end++) {
        }
        token.value = line.substr(i, end - i);
        token.type = IDENTIFIER;
        tokens.push_back(token);
        i = end;
        break;
      }
    }
  }

  return true;
}

void printTokens(vector<Token>& tokens) {
	char buf[100];
	for (int i = 0; i < tokens.size(); i++) {
		if (tokens[i].toString(buf) < 0) {
			LOG("Error getting string for token.\n");
		}
		LOG("%s\n", buf);
	}
}

class ASTNode {
 private:
  ASTNode* parent = NULL;
  vector<ASTNode*> children;
  Token token;

 public:
  ASTNode(ASTNode* parent) {
    this->parent = parent;
  }

  ASTNode(Token token, ASTNode* parent) {
    this->token = token;
    this->parent = parent;
  }

  ~ASTNode() {
    for (int i = 0; i < children.size(); i++) {
      delete children[i];
    }
  }

  ASTNode* getParent() const { return parent; }

  void setParent(ASTNode* parent) { this->parent = parent; }

  const vector<ASTNode*>& getChildren() { return children; }

  void addChild(ASTNode* astNode) { children.push_back(astNode); }

  const Token& getToken() const { return token; }

  void setToken(Token token) { this->token = token; }
};

bool parse(vector<Token>& tokens, ASTNode** root) {
  ASTNode* node = new ASTNode(NULL);
  *root = node;
  for (int i = 0; i < tokens.size(); i++) {
    Token token = tokens[i];
    ASTNode* child;
    switch (token.type) {
      case LIST_OPEN:
        child = new ASTNode(token, node);
        node->addChild(child);
        node = child;
        break;

      case LIST_CLOSE:
      	if (node->getParent() != NULL) {
      		node = node->getParent();
      	} else {
      		char buf[100];
      		token.toString(buf);
      		LOG("Unexpected ')'. %s\n", buf);
      		return false;
      	}
        break;

      default:
        child = new ASTNode(token, node);
        node->addChild(child);
        break;
    }
  }
  if (node != *root) {
  	LOG("Missing ')'\n");
  	return false;
  }
  return true;
}

void printTree(ASTNode* root) {
	LOG("AST Tree:\n");
	stack<pair<int, ASTNode*>> nodes;
	nodes.push(pair<int, ASTNode*>(0, root));
	string depthIndicator = "|";
	int lastDepth = 0;
	while (!nodes.empty()) {
		pair<int, ASTNode*> nodeAndDepth = nodes.top();
		nodes.pop();
		int depth = nodeAndDepth.first;
		ASTNode* node = nodeAndDepth.second;

		if (lastDepth > depth) {
			depthIndicator.pop_back();
			depthIndicator.pop_back();
		} 
		if (lastDepth < depth) {
			depthIndicator += " |";
		}

		lastDepth = depth;
		char buf[100];
		node->getToken().toString(buf);

		LOG("%s %s\n", depthIndicator.c_str(), buf);
		const vector<ASTNode*>& children = node->getChildren();
		for(int i = children.size()-1; i >= 0; i--) {
			nodes.push(pair<int, ASTNode*> (depth +1, children[i]));
		}
	}
}

static int editorRepl() {
	vector<Token> tokens; 
	string line;
	bool run = true;
	while (run) {
		printf("> ");
		if (!getline(cin, line)) {
			run = false;
			continue;
		}
		tokens.clear();
		if (!tokenize(line, tokens)) {
			LOG("Failed to tokenize: %s\n", line.c_str());
			continue;
		}
		printTokens(tokens);
		ASTNode* root;
		if (!parse(tokens, &root)) {
			LOG("Parse error!\n");
			continue;
		}

		printTree(root);
	}
	return 0;
}

int main(int argc, char* args[]) {
  LOG("%s Version %d.%d\n", args[0], Game_VERSION_MAJOR, Game_VERSION_MINOR);

  LOG("SDL Compiled Version: %d\n", SDL_COMPILEDVERSION);

  if (!initSystem()) {
    LOG("Failed to initialize system! Exiting...\n");
    return 1;
  }

  int returnVal = editorRepl();

  // Quit SDL subsystems
  SDL_Quit();

  return returnVal;
}