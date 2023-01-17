#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "ShapeSkin.h"
#include "GLSL.h"
#include "Program.h"
#include "TextureMatrix.h"

using namespace std;
using namespace glm;

ShapeSkin::ShapeSkin() :
	prog(NULL),
	elemBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0)
{
	T = make_shared<TextureMatrix>();
}

ShapeSkin::~ShapeSkin()
{
}

void ShapeSkin::setTextureMatrixType(const std::string &meshName)
{
	T->setType(meshName);
}

void ShapeSkin::loadMesh(const string &meshName)
{
	// Load geometry
	// This works only if the OBJ file has the same indices for v/n/t.
	// In other words, the 'f' lines must look like:
	// f 70/70/70 41/41/41 67/67/67
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = attrib.vertices;
		norBuf = attrib.normals;
		texBuf = attrib.texcoords;

		// convert posBuf into vector of vec3's
		for(int i = 0; i < posBuf.size(); i+=3) {
			initialVertPos.push_back(glm::vec3(posBuf[i], posBuf[i + 1], posBuf[i + 2]));
		}
		// convert norBuf into vector of vec3's
		for(int i = 0; i < norBuf.size(); i+=3) {
			initialVertNor.push_back(glm::vec3(norBuf[i], norBuf[i + 1], norBuf[i + 2]));
		}

		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t &mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for(size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
					elemBuf.push_back(idx.vertex_index);
				}
				index_offset += fv;
				// per-face material (IGNORE)
				//shapes[s].mesh.material_ids[f];
			}
		}
	}
}

void ShapeSkin::loadAttachment(const std::string &filename)
{
	ifstream in;
	in.open(filename);
	if(!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;

	string line;
	bool firstLine = true;
	int maxInfluences = 0;
	while(1) {
		getline(in, line);
		if(in.eof()) {
			break;
		}
		if(line.empty()) {
			continue;
		}
		// Skip comments
		if(line.at(0) == '#') {
			continue;
		}
		
		// create temp index + weight vecs
		vector<int> tempIndVec;
		vector<float> tempWgtVec;

		// Parse lines
		string value;
		stringstream ss(line);

		// Handle vertCount boneCount maxInfluences line
		if(firstLine) {
			firstLine = false;
			ss >> value; // vertCount
			ss >> value; // boneCount
			ss >> value; // maxInfluences
			maxInfluences = stoi(value);
			continue;
		}

		// Determine vert specific influence count
		ss >> value;
		int curInfluences = stoi(value);

		// Populate temp vecs
		for(int i = 0; i < curInfluences; i++) {
			// Parse index
			ss >> value;
			tempIndVec.push_back(stoi(value));
			// Parse weight
			ss >> value;
			tempWgtVec.push_back(stof(value));
		}

		// Pad temp vecs
		for(int i = 0; i < (maxInfluences - curInfluences); i++) {
			tempIndVec.push_back(0);
			tempWgtVec.push_back(0.0f);
		}

		// Store in attribute vecs
		boneIndVec.push_back(tempIndVec);
		boneWgtVec.push_back(tempWgtVec);
	}

	in.close();
}

void ShapeSkin::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	
	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	
	// Send the element array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::update(int k, vector<glm::mat4> bindMats, vector<vector<glm::mat4>> allFrameMats)
{
	// Get the bone Mat4's for the kth frame
	vector<glm::mat4> frameMats = allFrameMats[k];

	// Compute (Mj(k) * M(0)^-1) for all bones
	vector<glm::mat4> matProductVec;
	for(int i = 0; i < frameMats.size(); i++) {
		matProductVec.push_back(frameMats[i] * bindMats[i]);
	}

	// Compute new vertex coords
	posBuf.clear();
	for(int i = 0; i < boneIndVec.size(); i++) { // For all vertices (i)
		
		glm::vec3 x_ik = glm::vec3(0.0f, 0.0f, 0.0f);

		// x_i(0) with homogeneous coord
		glm::vec4 x_io = glm::vec4(initialVertPos[i][0], initialVertPos[i][1], initialVertPos[i][2], 1.0f);

		for(int itr = 0; itr < boneIndVec[i].size(); itr++) { // For all influenced bones (j)

			int j = boneIndVec[i][itr];
			glm::vec4 prod = matProductVec[j] * x_io;
			prod = boneWgtVec[i][itr] * prod;
		
			x_ik[0] += prod[0];
			x_ik[1] += prod[1];
			x_ik[2] += prod[2];
		}

		// Add to buffer
		posBuf.push_back(x_ik[0]);
		posBuf.push_back(x_ik[1]);
		posBuf.push_back(x_ik[2]);
	}

	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_DYNAMIC_DRAW);
	
	// Compute new normals
	norBuf.clear();
	for(int i = 0; i < boneIndVec.size(); i++) { // For all vertices (i)
		
		glm::vec3 n_ik = glm::vec3(0.0f, 0.0f, 0.0f);

		// n_i(0) with homogeneous coord
		glm::vec4 n_io = glm::vec4(initialVertNor[i][0], initialVertNor[i][1], initialVertNor[i][2], 0.0f);

		for(int itr = 0; itr < boneIndVec[i].size(); itr++) { // For all influenced bones (j)

			int j = boneIndVec[i][itr];
			glm::vec4 prod = matProductVec[j] * n_io;
			prod = boneWgtVec[i][itr] * prod;
			
			n_ik[0] += prod[0];
			n_ik[1] += prod[1];
			n_ik[2] += prod[2];
		}

		// Renormalize
		n_ik = glm::normalize(n_ik);

		// Add to buffer
		norBuf.push_back(n_ik[0]);
		norBuf.push_back(n_ik[1]);
		norBuf.push_back(n_ik[2]);
	}

	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_DYNAMIC_DRAW);

	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::draw(int k) const
{
	assert(prog);

	// Send texture matrix
	glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T->getMatrix()));
	
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}
