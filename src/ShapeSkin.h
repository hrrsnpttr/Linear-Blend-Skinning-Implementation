#pragma once
#ifndef SHAPESKIN_H
#define SHAPESKIN_H

#include <memory>
#include <fstream>

#define GLEW_STATIC
#include <GL/glew.h>

class MatrixStack;
class Program;
class TextureMatrix;

class ShapeSkin
{
public:
	ShapeSkin();
	virtual ~ShapeSkin();
	void setTextureMatrixType(const std::string &meshName);
	void loadMesh(const std::string &meshName);
	void loadAttachment(const std::string &filename);
	void setProgram(std::shared_ptr<Program> p) { prog = p; }
	void init();
	void update(int k, std::vector<glm::mat4> bindMats, std::vector<std::vector<glm::mat4>> allFrameMats);
	void draw(int k) const;
	void setTextureFilename(const std::string &f) { textureFilename = f; }
	std::string getTextureFilename() const { return textureFilename; }
	std::shared_ptr<TextureMatrix> getTextureMatrix() { return T; }
	
private:
	std::shared_ptr<Program> prog;
	std::vector<unsigned int> elemBuf;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	std::vector<std::vector<int>> boneIndVec;   // Bone indices
	std::vector<std::vector<float>> boneWgtVec; // Bone weights
	std::vector<glm::vec3> initialVertPos;      // x_i(0) for all verts
	std::vector<glm::vec3> initialVertNor;      // n_i(0) for all verts
	GLuint elemBufID;
	GLuint posBufID;
	GLuint norBufID;
	GLuint texBufID;
	std::string textureFilename;
	std::shared_ptr<TextureMatrix> T;
};

#endif
