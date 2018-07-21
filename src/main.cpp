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
#include <functional>
#include <cctype>
#include <set>
#include <iostream>
#include "json.h"
#include <fstream>
#include "Box2D/Box2D.h"

using json = nlohmann::json;

#define SPRITE_COUNT 500

const int targetFrameDuration = 1000 / 200;  // 200 frames per sec

SDL_Rect rects[SPRITE_COUNT];

bool debugDraw;


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


bool initSystem() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    LOG("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  return initRenderingSystem();
}

class AnimationFrame {
private:
	int duration;
	SDL_Rect frame;

public:
	AnimationFrame(int duration, SDL_Rect frame) {
		this->duration = duration;
		this->frame = frame;
	}

	const SDL_Rect& getFrame() const {
		return frame;
	}

	int getDuration() const {
		return duration;
	}
};

class AnimationData {
private:
	unique_ptr<vector<unique_ptr<const AnimationFrame>>> frames;
	string texturePath;

public:
	AnimationData(string texturePath,  unique_ptr<vector<unique_ptr<const AnimationFrame>>> frames) {
		this->texturePath = texturePath;
		this->frames = move(frames);
	}

	const vector<unique_ptr<const AnimationFrame>>& getFrames() const {
		return *frames;
	}

	const string getTexturePath() const {
		return texturePath;
	}
};

bool loadAnimationData(string path, unique_ptr<AnimationData>* animationData) {
	LOG_STREAM() << "Loading AnimationData for: " << path << endl;
	std::ifstream i(path);
	json animationJson;
	i >> animationJson;

	auto framesItr = animationJson.find("frames");
	if (framesItr == animationJson.end()) {
		LOG_STREAM() << "Error! Can't find 'frames' in animation json." << endl;
		return false;
	}

	auto frames = framesItr.value();
	if (!frames.is_array()) {
		LOG_STREAM() << "Error! Expected 'frames' to be an array." << endl;
		return false;
	}

	unique_ptr<vector<unique_ptr<const AnimationFrame>>> animationFrames = 
		unique_ptr<vector<unique_ptr<const AnimationFrame>>>(new vector<unique_ptr<const AnimationFrame>>());

	for (int i = 0; i < frames.size(); i++) {
		json frame = frames[i];

		// TODO make robust to failure
		int duration =  frame.at("duration");

		json frameData = frame.at("frame");

		SDL_Rect rect;

		rect.x = frameData.at("x");
		rect.y = frameData.at("y");
		rect.w = frameData.at("w");
		rect.h = frameData.at("h");


		animationFrames->push_back(unique_ptr<AnimationFrame>(new AnimationFrame(duration, rect)));
	}

 	string texturePath = animationJson.at("meta").at("image");
	*animationData = unique_ptr<AnimationData> (new AnimationData(texturePath, move(animationFrames)));

	return true;
}

void createTile(b2World& world, TileMap& tileMap, int x, int y, shared_ptr<TileInstance> tileInstance) {
	tileMap.set(x, y, tileInstance);

	int tileLength = tileMap.getTileLength();

	b2BodyDef groundBodyDef;
	LOG("New Tile at X(%f), Y(%f)\n", x * tileLength * PIXELS_TO_B2_UNITS, -y * tileLength * PIXELS_TO_B2_UNITS);
	groundBodyDef.position.Set(x * tileLength * PIXELS_TO_B2_UNITS, -y * tileLength * PIXELS_TO_B2_UNITS);

	// Call the body factory which allocates memory for the ground body
	// from a pool and creates the ground box shape (also from a pool).
	// The body is also added to the world.
	b2Body* groundBody = world.CreateBody(&groundBodyDef);

	// Define the ground box shape.
	b2PolygonShape groundBox;

	// The extents are the half-widths of the box.
	groundBox.SetAsBox(tileLength * PIXELS_TO_B2_UNITS/2.0f, tileLength * PIXELS_TO_B2_UNITS/2.0f);

	// Add the ground fixture to the ground body.
	groundBody->CreateFixture(&groundBox, 0.0f);
}

void loadImage(const string& imgPath) {
	if (!initSystem()) {
	  LOG("Failed to initialize system! Exiting...\n");
	  return;
	}

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

	shared_ptr<Font> font;
	if (!assetManager->getFont("../assets/arial_regular_10", &font)) {
	  LOG("Failed to load font\n");
	  return;
	}

	// Current displayed texture
	shared_ptr<Texture> texture = shared_ptr<Texture>(NULL);

	if (!assetManager->getTexture(imgPath, &texture)) {
	  LOG("Failed to load texture! Exiting...\n");
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

    // Clear screen
    renderer->clear();

    renderer->render(*texture, NULL, NULL);

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

	// Quit SDL subsystems
	SDL_Quit();
}

void runGame() {
	if (!initSystem()) {
	  LOG("Failed to initialize system! Exiting...\n");
	  return;
	}

	SDL_Window* window;

	if (!createWindow(&window)) {
		LOG("Failed to create window!\n");
		return;
	}

	// Define the gravity vector.
	b2Vec2 gravity(0.0f, -1.0f);

	// Construct a world object, which will hold and simulate the rigid bodies.
	b2World world(gravity);

  shared_ptr<Renderer> renderer = shared_ptr<Renderer>(NULL);

  if (!createRenderer(window, &renderer)) {
    LOG("Failed to create system! Exiting...\n");
    return;
  }

  world.SetDebugDraw(renderer.get());
  renderer->SetFlags(b2Draw::e_shapeBit);

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

  unique_ptr<AnimationData> animationData;
	if(!loadAnimationData("../assets/Walk.json", &animationData)) {
		LOG("Could not load animation!\n");
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

  createTile(world, tileMap, 4, 4, tileInstance);
  createTile(world, tileMap, 4, 5, tileInstance);
  createTile(world, tileMap, 4, 6, tileInstance);
  createTile(world, tileMap, 5, 6, tileInstance);
  createTile(world, tileMap, 6, 6, tileInstance);
  createTile(world, tileMap, 7, 6, tileInstance);

  createTile(world, tileMap, 9, 6, tileInstance);
  createTile(world, tileMap, 10, 6, tileInstance);
  createTile(world, tileMap, 11, 6, tileInstance);
  createTile(world, tileMap, 12, 6, tileInstance);
  createTile(world, tileMap, 12, 5, tileInstance);
  createTile(world, tileMap, 12, 4, tileInstance);
  createTile(world, tileMap, 13, 4, tileInstance);
  createTile(world, tileMap, 14, 4, tileInstance);

  SDL_Rect playerDestination{0, 0, texture->getWidth(), texture->getHeight()};
  float playerSpeed = 200.0f / 10000;  // per ms

  // Define the dynamic body. We set its position and call the body factory.
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(200.0f * PIXELS_TO_B2_UNITS, -150.0f * PIXELS_TO_B2_UNITS);
	bodyDef.gravityScale = 1.0f;
	b2Body* playerBody = world.CreateBody(&bodyDef);

	// Define another box shape for our dynamic body.
	b2PolygonShape dynamicBox;
	LOG("Texture Width(%d) Height(%d)\n", texture->getWidth(), texture->getHeight());
	dynamicBox.SetAsBox(texture->getWidth() * PIXELS_TO_B2_UNITS/2.0f, texture->getHeight() * PIXELS_TO_B2_UNITS/2.0f);

	// Define the dynamic body fixture.
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicBox;

	// Set the box density to be non-zero, so it will be dynamic.
	fixtureDef.density = 20.0f;

	// Override the default friction.
	fixtureDef.friction = 0.3f;

	// Add the shape to the body.
	playerBody->CreateFixture(&fixtureDef);


	float32 timeStep = 1.0f / 60.0f;
	int32 velocityIterations = 6;
	int32 positionIterations = 2;

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

      if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_F1) {
	    	if (! debugDraw) {
	    		debugDraw = true;
	    		renderer->SetFlags(b2Draw::e_aabbBit | b2Draw::e_centerOfMassBit);
	    	} else if (renderer->GetFlags() & b2Draw::e_aabbBit) {
	    		renderer->SetFlags(b2Draw::e_shapeBit | b2Draw::e_centerOfMassBit);
	    	} else {
	    		debugDraw = false;
	    	}
      }
    }

    const Uint8* state = SDL_GetKeyboardState(NULL);

    b2Vec2 velocity;
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

    b2Vec2 p = playerBody->GetWorldPoint(b2Vec2(0.0f, 0.0f));
    playerBody->ApplyLinearImpulse(playerBody->GetWorldVector(velocity), p, true);

    world.Step(timeStep, velocityIterations, positionIterations);

    playerDestination.x = playerBody->GetPosition().x * B2_UNITS_TO_PIXELS - 16;
    playerDestination.y = -playerBody->GetPosition().y * B2_UNITS_TO_PIXELS - 16;
    //LOG_STREAM() << "Player Destination: X(" << playerDestination.x << ") Y(" << playerDestination.y << ")" << endl;

    // Clear screen
    renderer->clear();

    tileMap.render(*renderer, 0, 0);

    renderer->render(*texture, NULL, &playerDestination);

    font->render(*renderer, fpsText, 10, 10);

    if (debugDraw) {
	    world.DrawDebugData();
	  }

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

  // Quit SDL subsystems
  SDL_Quit();
}

enum TokenType {
	LIST_OPEN = 0,
	LIST_CLOSE,
	IDENTIFIER_TOKEN,
	STRING_TOKEN
};

enum LexVal {
	OPEN_PAREN = 0,
	CLOSE_PAREN,
	SPACE,
	ALPHA_NUMERIC,
	QUOTE
};

string nodeTypeStrings[] = {
	"LIST_NODE",
	"IDENTIFIER_NODE",
	"STRING_NODE"
};

enum NodeType {
	LIST_NODE = 0,
	IDENTIFIER_NODE,
	STRING_NODE
};

enum ValueType {
	STRING_VALUE = 0,
	CELL_VALUE,
	NIL_VALUE,
	FUNCTION_VALUE
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

        token.value = line.substr(i+1, quoteEnd - i - 1);
        token.type = STRING_TOKEN;
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
        token.type = IDENTIFIER_TOKEN;
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

class Value {

public:
	virtual ValueType getValueType() const = 0;

	virtual ostream& insertIntoStream(ostream &out) const = 0;

	friend ostream& operator<< (ostream &out, const Value &c) {
		return c.insertIntoStream(out);
	}

};

class ASTNode {
 private:
  ASTNode* parent = NULL;
  vector<ASTNode*> children;
  Token token;
  NodeType nodeType;
  shared_ptr<Value> value;

 public:
  ASTNode(Token token, NodeType nodeType, ASTNode* parent)
      : ASTNode(token, nodeType, shared_ptr<Value>(nullptr), parent) {}

  ASTNode(Token token, NodeType nodeType, shared_ptr<Value> value,
          ASTNode* parent) {
    this->token = token;
    this->value = value;
    this->parent = parent;
    this->nodeType = nodeType;
  }

  ~ASTNode() {
    for (int i = 0; i < children.size(); i++) {
      delete children[i];
    }
  }

  ASTNode* getParent() const { return parent; }

  void setParent(ASTNode* parent) { this->parent = parent; }

  const vector<ASTNode*>& getChildren() const { return children; }

  void addChild(ASTNode* astNode) { children.push_back(astNode); }

  const Token& getToken() const { return token; }

  const shared_ptr<const Value> getValue() const {
  	return value;
  }

  const shared_ptr<Value> getValue() {
  	return value;
  }

  NodeType getNodeType() const {
  	return nodeType;
  }
};

class NilValue : public Value {
private:

public:

	ValueType getValueType() const {
		return NIL_VALUE;
	}

	ostream& insertIntoStream(ostream &out) const {
		out << "nil";
		return out;
	}
};

class FunctionValue : public Value {
private:
	const ASTNode* node;
	function<bool(stack<shared_ptr<Value>>&)> cppFn;
	bool isCppFn_;


public:
	FunctionValue(function<bool(stack<shared_ptr<Value>>&)> cppFn) {
		isCppFn_ = true;
		this->cppFn = cppFn;
	}

	FunctionValue(const ASTNode* node) {
		isCppFn_ = false;
		this->node = node;
	}

	~FunctionValue() {
		delete node;
	}

	ValueType getValueType() const {
		return FUNCTION_VALUE;
	}

	ostream& insertIntoStream(ostream &out) const {
		out << (isCppFn_ ? "cppFn" : "fn");
		return out;
	}

	const ASTNode* getNode() const {
		return node;
	}

	bool isCppFn() const {
		return isCppFn_;
	}

	function<bool(stack<shared_ptr<Value>>&)> getCppFn() const {
		return cppFn;
	}

};

class StringValue : public Value {
private:
	unique_ptr<string> value;

public:
	StringValue(unique_ptr<string> value) {
		this->value = move(value);
	}

	ValueType getValueType() const {
		return STRING_VALUE;
	}

	const string& getValue() const {
		return *value;
	}

	ostream& insertIntoStream(ostream &out) const {
		out << "\"" << *value << "\"";
		return out;
	}
};

class CellValue : public Value {
private:
	shared_ptr<Value> value;
	shared_ptr<CellValue> next;

public:
	CellValue() {
		value = shared_ptr<Value>(NULL);
		next = shared_ptr<CellValue>(NULL);
	}

	CellValue(shared_ptr<Value> value, shared_ptr<CellValue> next) {
		this->value = move(value);
		this->next = next;
	}

	ValueType getValueType() const {
		return CELL_VALUE;
	}

	bool hasValue() const {
		return value.get() != NULL;
	}

	const Value& getValue() const {
		return *value;
	}

	void setValue(shared_ptr<Value> value) {
		this->value = value;
	}

	shared_ptr<CellValue> getNext() {
		return next;
	}

	bool hasNext() const {
		return next.get() == NULL;
	}

	void insertIntoStreamRaw(ostream &out) const {
		if (value.get() == NULL) {
			return;
		}

		out << *value;

		if (next.get() != NULL) {
			if (next->hasValue()) {
				out << " ";
			}
			next->insertIntoStreamRaw(out);
		}
	}

	ostream& insertIntoStream(ostream &out) const {
		out << "(";
		insertIntoStreamRaw(out);
		out << ")";
		return out;
	}

};


bool parseStmt(vector<Token>& tokens, unique_ptr<ASTNode>& root) {
  if (tokens.empty()) {
  	return true;
  }

  ASTNode* node = NULL;
  bool finished = false;
  int i;

  for (i = 0; i < tokens.size() && !finished; i++) {
    Token token = tokens[i];
    switch (token.type) {
      case LIST_OPEN: {
        ASTNode* child = new ASTNode(token, LIST_NODE, node);
        if (node == NULL) {
          root = unique_ptr<ASTNode>(child);
        } else {
          node->addChild(child);
        }
        node = child;
        break;
      }

      case LIST_CLOSE: {
        node = node->getParent();

        finished = node == NULL;

        break;
      }
      case STRING_TOKEN: {
        ASTNode* child = new ASTNode(
            token, STRING_NODE,
            shared_ptr<Value>(new StringValue(
                unique_ptr<string>(new string(token.value)))),
            node);
        if (node == NULL) {
          root = unique_ptr<ASTNode>(child);
          finished = true;
        } else {
          node->addChild(child);
        }
        break;
      }
      case IDENTIFIER_TOKEN: {
      	ASTNode* child = new ASTNode(
      	    token, IDENTIFIER_NODE, node);
      	if (node == NULL) {
      	  root = unique_ptr<ASTNode>(child);
      	  finished = true;
      	} else {
      	  node->addChild(child);
      	}
      	break;
      }
    }
  }
  if (node != NULL) { 
  	LOG("Missing ')'\n");
  	return false;
  }

  if (i < tokens.size()) {
  	LOG("Unexpected token: %s\n", tokens[i].value.c_str());
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

    ostream& stream = LOG_STREAM() << depthIndicator << " " 
                 << "NodeType: " << nodeTypeStrings[node->getNodeType()];
   	if (node->getValue().get() != nullptr) {
   		stream << ", Value: " << *(node->getValue());
   	}
   	if (node->getToken().type == IDENTIFIER_TOKEN) {
   		stream << ", " << node->getToken().value;
   	}
    stream <<  endl;

    const vector<ASTNode*>& children = node->getChildren();
    for (int i = children.size() - 1; i >= 0; i--) {
      nodes.push(pair<int, ASTNode*>(depth + 1, children[i]));
    }
  }
}

void printStackDestructive(stack<shared_ptr<Value>>& stak) {
	stack<shared_ptr<Value>> backwardsStack;
	LOG_STREAM() << "Stack:" << endl;
	while(!stak.empty()) {
		shared_ptr<Value> top = stak.top();
		stak.pop();
		backwardsStack.push(top);
		LOG_STREAM() << *top << endl;
	}

	while(!backwardsStack.empty()) {
		shared_ptr<Value> top = backwardsStack.top();
		backwardsStack.pop();

		stak.push(top);
	}
}

bool cons(stack<shared_ptr<Value>>& operands) {
	if (operands.size() < 2) {
		LOG("Expecting two operands to cons! Found %lu\n", operands.size());
		return false;
	}
	shared_ptr<Value> op1 = operands.top();
	operands.pop();

	shared_ptr<Value> op2 = operands.top();
	operands.pop();

	if (op2->getValueType() != CELL_VALUE) {
		LOG_STREAM() << "Expecting LIST as 2nd arg to cons. Found " << *op2 << endl;
		return false;
	}

	shared_ptr<CellValue> cdr = dynamic_pointer_cast<CellValue>(op2);
	
	operands.push(shared_ptr<Value>(new CellValue(op1, cdr)));

	return true;
}

enum OpType {
	FUN_CALL = 0,
};

bool eval(map<string, shared_ptr<Value>> env, const ASTNode* root,
          stack<shared_ptr<Value>>& operands) {
  stack<const ASTNode*> nodes;
  stack<OpType> operations;
  set<const ASTNode*> processedNodes;

  nodes.push(root);

  while (!nodes.empty()) {
    const ASTNode* node = nodes.top();

    if (processedNodes.find(node) == processedNodes.end()) {
    	processedNodes.insert(node);

    	switch (node->getNodeType()) {
    	  case STRING_NODE: {
    	    nodes.pop();
    	    operands.push(shared_ptr<Value>(new StringValue(
    	        unique_ptr<string>(new string(node->getToken().value)))));
    	    break;
    	  }
    	  case IDENTIFIER_NODE: {
    	  	nodes.pop();
    	  	auto keyAndVal = env.find(node->getToken().value);
    	  	if (keyAndVal == env.end()) {
    	  		LOG("No value named: %s\n", node->getToken().value.c_str());
    	  		return false;
    	  	}

    	  	operands.push(keyAndVal->second);
    	  	break;
    	  }
    	  case LIST_NODE: {
    	  	// test for special forms


    	  	const vector<ASTNode*>& children = node->getChildren();
    	  	if (children.empty()) {
    	  		operands.push(shared_ptr<Value>(new CellValue()));
    	  		nodes.pop();
    	  	} else {
    	  		operations.push(FUN_CALL);
    	  		for (int i = 0; i < children.size(); i++) {
    	  			nodes.push(children[i]);
    	  		}
    	  	}
    	  }
    	}
    } else {
    	nodes.pop();
    	OpType op = operations.top();
    	operations.pop();

    	switch (op) {
    		case FUN_CALL: {
    			shared_ptr<Value> val = operands.top();
    			operands.pop();
    			if (val->getValueType() != FUNCTION_VALUE) {
    				LOG_STREAM() << "Expected function call, but got " << *val << endl;
    				printStackDestructive(operands);
    				return false;
    			}
    			shared_ptr<FunctionValue> fn = dynamic_pointer_cast<FunctionValue> (val);
    			if (fn->isCppFn()) {
    				if (!fn->getCppFn()(operands)) {
    					LOG("Cpp fn failed! \n");
    					return false;
    				}
    			} else {
    				nodes.push(fn->getNode());
    			}
    		}
    	}
    }
  }

  return true;
}

bool run(stack<shared_ptr<Value>>& operands) {

	runGame();

	operands.push(shared_ptr<Value>(new NilValue()));

	return true;
}

bool loadImg(stack<shared_ptr<Value>>& operands) {
	if (operands.size() < 1) {
		LOG("Expecting 1 operands to load-img! Found %lu\n", operands.size());
		return false;
	}
	shared_ptr<Value> op = operands.top();
	operands.pop();

	if (op->getValueType() != STRING_VALUE) {
		LOG_STREAM() << "Expecting STRING as arg to load-img. Found " << *op << endl;
		return false;
	}


	shared_ptr<StringValue> str = dynamic_pointer_cast<StringValue>(op);

	loadImage(str->getValue());

	operands.push(shared_ptr<Value>(new NilValue()));

	return true;
}

static int editorRepl() {
	map<string, shared_ptr<Value>> env;

	env.emplace("run", shared_ptr<Value>(new FunctionValue(&run)));
	env.emplace("cons", shared_ptr<Value>(new FunctionValue(&cons)));
	env.emplace("load-img", shared_ptr<Value>(new FunctionValue(&loadImg)));


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
		unique_ptr<ASTNode> root = NULL;
		if (!parseStmt(tokens, root)) {
			LOG("Parse error!\n");
			continue;
		}

		if (root == NULL) {
			continue;
		}

		printTree(root.get());

		stack<shared_ptr<Value>> operands;
		if(!eval(env, root.get(), operands)) {
			LOG("Eval failed!\n");
			continue;
		}
		printStackDestructive(operands);

		if (!operands.empty()) {
			shared_ptr<Value> val = operands.top();
			operands.pop();
			cout << *val << endl;
		}
	}
	return 0;
}

int main(int argc, char* args[]) {
  LOG("%s Version %d.%d\n", args[0], Game_VERSION_MAJOR, Game_VERSION_MINOR);

  LOG("SDL Compiled Version: %d\n", SDL_COMPILEDVERSION);

  int returnVal = editorRepl();

  return returnVal;
}