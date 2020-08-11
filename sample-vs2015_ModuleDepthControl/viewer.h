#pragma once

#include <iostream>
#include <vector>
#include <map>
#include "GL/glew.h"
#include "GLFW/glfw3.h"


class Clouds
{
public:

	Clouds();
	~Clouds();

	void UploadData(float* cloud, int cloud_size, int data_stride, int color_offset, int *in_index, int in_numIdx);
	void DrawPoints(float* projMtx, float* viewMtx, float pointSize, float useWhiteColor);

private:
	static const char* vsCode;
	static const char* fsCode;

	GLuint ibo;
	GLuint vbo;
	GLuint prog;
	int projMtxLoc;
	int mvMtxLoc;
	int sizeLoc;
	int useWhiteLoc;

	int offset;
	int stride;

	int numIndices;
	int numPoints;
	bool hasTriangles;
};

class BackgroundTex
{
public:

	BackgroundTex();
	~BackgroundTex();

	void UploadTex(char* imgData, int w, int h);
	void Draw();

private:
	static const char* vsCode;
	static const char* fsCode;

	GLuint tex;
	GLuint ibo;
	GLuint vbo;
	GLuint prog;
	int texRegLoc;

	const int stride = 20;	// 5 float
	const int offset = 3;
	const int numVert = 4;
	const int numIndices = 6;
};


class PointCloudViewer
{
public:
	PointCloudViewer(float focalLength);
	~PointCloudViewer();

	void UpdateCamera(float *poseMtx, bool transpose);
	void UpdateBackground(char* color, int w, int h);
	void UpdateGeometry(float *in_pointsCloud, int in_numPoints, int vertStride, int sector_num, int *sector_id_list, int *sector_vert_num, int *sector_mesh_id_num, int *in_index, int in_numIdx, bool enable_sector);
	void Draw(float pointSize, float useWhiteColor);

	bool shutDown = false;

private:
	void _SetPerspectiveLH(float focalLength, float aspect, float nearPlane, float farPlane);
	void _SetPerspectiveRH(float focalLength, float aspect, float nearPlane, float farPlane);
	//void _SetPerspectiveLH(float fovDeg, float aspect, float nearPlane, float farPlane);
	//void _SetPerspectiveRH(float fovDeg, float aspect, float nearPlane, float farPlane);

	void RemoveAllClouds();

private:
	GLFWwindow* _win;	
	std::map<int, Clouds*> clouds;
	BackgroundTex* bg = nullptr;
	float projMtx[16];
	float viewMtx[16];

	bool prev_enable_sector = false;
	bool prev_contain_idx = false;
};
