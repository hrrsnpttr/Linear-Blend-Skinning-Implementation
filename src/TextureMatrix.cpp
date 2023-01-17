#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "TextureMatrix.h"

using namespace std;
using namespace glm;

TextureMatrix::TextureMatrix()
{
	type = Type::NONE;
	T = mat3(1.0f);
	xOffset = 0.0f;
	yOffset = 0.0f;
}

TextureMatrix::~TextureMatrix()
{
	
}

void TextureMatrix::setType(const string &str)
{
	if(str.find("Body") != string::npos) {
		type = Type::BODY;
	} else if(str.find("Mouth") != string::npos) {
		type = Type::MOUTH;
	} else if(str.find("Eyes") != string::npos) {
		type = Type::EYES;
	} else if(str.find("Brows") != string::npos) {
		type = Type::BROWS;
	} else {
		type = Type::NONE;
	}
}

void TextureMatrix::update(unsigned int key)
{
	// Update T 
	if(type == Type::BODY) {
		// Do nothing
	} else if(type == Type::MOUTH) {
		if(key == 109) { // m
			xOffset += 0.1f;
			if (xOffset > 0.2f) { xOffset = 0.0f; }
		}
		if(key == 77) { // M
			yOffset += 0.1f;
			if (yOffset >= 1.1f) { yOffset = 0.1f; }
		}

	} else if(type == Type::EYES) {
		if(key == 101) { // e
			xOffset += 0.2f;
			if (xOffset > 0.4f) { xOffset = 0.0f; }
		}
		if(key == 69) { // E
			yOffset += 0.1f;
			if (yOffset >= 1.1f) { yOffset = 0.1f; }
		}

	} else if(type == Type::BROWS) {
		if(key == 98) { // b
			yOffset += 0.1f;
			if (yOffset >= 1.1f) { yOffset = 0.1f; }
		}
	}

	// Apply Translation
	T[2][0] = xOffset;
	T[2][1] = yOffset;
}
