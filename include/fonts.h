#ifndef FONTS_INCLUDED
#define FONTS_INCLUDED


#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include "tinyxml2.h"
#include <memory>
#include <map>
#include "common.h"
#include "render.h"
#include "assets.h"

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
	shared_ptr<Texture> texture;
	int height;

public:
	Font(unique_ptr<StringToSharedGlyphPtrMap> glyphMap, shared_ptr<Texture> texture, int height) {
		this->glyphMap = std::move(glyphMap);
		this->texture = texture;
		this->height = height;
	}

	void render(const Renderer& renderer, const string& text, int x, int y) {
		y += height / 2;
		for (int i = 0; i < text.size(); i++) {
			string key = string(1, text.at(i));
			shared_ptr<Glyph> glyph = glyphMap->at(key);
			
			shared_ptr<SDL_Rect> source = glyph->getRect();
			SDL_Rect destination = { x + glyph->getOffsetX(), y - glyph->getOffsetY(), source->w, source->h };
			
			renderer.render(*texture, source.get(), &destination);

			x += glyph->getAdvance();
		}
	}
};

bool loadFont(string path, AssetManager* assetManager, shared_ptr<Font>* font) {

	LOG("Loading Font: %s\n", path.c_str());
	string texPath = path;
	texPath += ".png";

	shared_ptr<Texture> texture = shared_ptr<Texture>(NULL);
	if (!assetManager->getTexture(texPath, &texture)) {
		LOG("Unable to load font texture!\n");
		return false;		
	}

	path += ".xml";
	XMLDocument doc;
	doc.LoadFile(path.c_str());
	XMLNode* fontNode = doc.FirstChildElement("font");
	if (!fontNode) {
		return false;
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
	
	*font = shared_ptr<Font>(new Font(std::move(valueToGlyphMap), texture, height));
	return true;
}

#endif