#pragma once

class DepthColorTex
{
public:

	DepthColorTex(int width, int height);
	~DepthColorTex();

	void UploadTex(void* imgData, int w, int h);
	void Draw();

private:
	static const char* vsCode;
	static const char* fsCode;
	pangolin::GlSlProgram* _prog = nullptr;
	pangolin::GlTexture _depthTex;

	GLuint _tex;
	GLuint _ibo;
	GLuint _vbo;
	int _texRegLoc;

	const int stride = 20;	// 5 float
	const int offset = 3;
	const int numVert = 4;
	const int numIndices = 6;
};

class Clouds
{
public:

	Clouds();
	~Clouds();

	void UpdateData(float* cloud, int cloud_size, int data_stride, int color_offset, int *in_index, int in_numIdx, GLuint priType = GL_TRIANGLES);	
	void DrawPoints(float* projMtx, float* viewMtx, float pointSize, float useWhiteColor);

private:
	static const char* vsCode;
	static const char* fsCode;
	pangolin::GlSlProgram* _prog = nullptr;
	GLuint _ibo;
	GLuint _vbo;
	int _projMtxLoc;
	int _mvMtxLoc;
	int _sizeLoc;
	int _useWhiteLoc;

	int _offset;
	int _stride;

	int _numIndices;
	int _numPoints;
	bool _hasTriangles;
	GLuint _primitiveType;
};