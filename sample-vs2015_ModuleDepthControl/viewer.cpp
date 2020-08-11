#include "viewer.h"

//#define FOCAL_LENGTH 247.630188f
#define VIEWER_W 640.0f
#define VIEWER_H 480.0f


GLuint CreateShaderProgramFromString(const std::string& vShader, const std::string& fShader, const char* name)
{
	// vertex shader first
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* p[1];
	p[0] = vShader.c_str();
	glShaderSource(vertexShader, 1, p, nullptr);
	glCompileShader(vertexShader);

	GLint isCompiled = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

		char* infoLog = new char[maxLength];
		glGetShaderInfoLog(vertexShader, maxLength, &maxLength, infoLog);
		std::cout << "vertex shader of program " << name << " compilation info: \n" << infoLog;
		delete[] infoLog;

		glDeleteShader(vertexShader);
		std::cout << "Create shader program error:" << name << std::endl;
		return -1;
	}

	// fragment shader 
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	p[0] = fShader.c_str();
	glShaderSource(fragmentShader, 1, p, nullptr);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

		char* infoLog = new char[maxLength];
		glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, infoLog);
		std::cout << "fragment shader of program " << name << " compilation info: \n" << infoLog;
		delete[] infoLog;

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		std::cout << "Create shader program error:" << name << std::endl;
		return -1;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);
	glBindAttribLocation(program, 0, "a_position");
	glBindAttribLocation(program, 1, "a_color");
	glBindAttribLocation(program, 2, "a_uv");
	glLinkProgram(program);


	GLint maxLength = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &maxLength);
	if (!maxLength)
	{
		char* infoLog = new char[maxLength];
        if (infoLog == nullptr) return -1;
		glGetProgramInfoLog(program, maxLength, &maxLength, infoLog);
		std::cout << name << ": Shader program compilation info: \n" << infoLog;
		std::cout << "Create shader program " << name << " error, abort" << std::endl << std::endl;
		delete[] infoLog;

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(program);
		return -1;
	}

	return program;
}

void Normalize(float* vec)
{
	float invLen = 1.0f / sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	vec[0] *= invLen;
	vec[1] *= invLen;
	vec[2] *= invLen;
}

float Dot(float* vec1, float* vec2)
{
	return (vec1[0] * vec2[0]) + (vec1[1] * vec2[1]) + (vec1[2] * vec2[2]);
}

//////////////////////////////////////////////////////////////////////////

const char* Clouds::vsCode =
"	#version 100 \n"
"	precision highp float;\n"
"	\n"
"	attribute vec3 a_position; \n"
"	attribute vec4 a_color; \n"
"	uniform mat4 projMtx; \n"
"	uniform mat4 mvMtx; \n"
"	uniform float pSizeScale; \n"
"	uniform float useWhite; \n"
"	varying vec4 color; \n"
"	void main()\n"
"	{\n"
"		gl_Position = projMtx * mvMtx * vec4(a_position, 1.0);\n"
"		vec4 vPos = mvMtx * vec4(a_position, 1.0);\n"
"		gl_PointSize = pSizeScale * abs(4.5 / vPos.z);	// Ethan: the further, the smaller \n"
"		color = (useWhite > 0.0)? vec4(1,1,1,1) : a_color;\n"
"	}\n";

const char* Clouds::fsCode =
"	#version 100 \n"
"	precision highp float;\n"
"	varying vec4 color;\n"
"	\n"
"	void main()\n"
"	{\n"
"		gl_FragData[0] = color * vec4(1.0, 1.0, 1.5, 1.0);\n"
"	}";

Clouds::Clouds()
{
	glGenBuffers(1, &ibo);
	glGenBuffers(1, &vbo);
	prog = CreateShaderProgramFromString(vsCode, fsCode, "");
	projMtxLoc = glGetUniformLocation(prog, "projMtx");
	mvMtxLoc = glGetUniformLocation(prog, "mvMtx");
	sizeLoc = glGetUniformLocation(prog, "pSizeScale");
	useWhiteLoc = glGetUniformLocation(prog, "useWhite");
	//std::cout << "prog:" << prog << ", locations: " << projMtxLoc << "," << mvMtxLoc << std::endl;
}


Clouds::~Clouds()
{
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);
}

void Clouds::UploadData(float* cloud, int cloud_size, int data_stride, int color_offset, int *in_index, int in_numIdx)
{
	numPoints = cloud_size;
	offset = color_offset;
	stride = data_stride;

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numPoints * stride, cloud, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	numIndices = in_numIdx;
	int* idx = in_index;
	hasTriangles = true;

	if (numIndices == 0)
	{
		hasTriangles = false;
		numIndices = numPoints;
		idx = new int[numIndices];
		for (int i = 0; i < numIndices; ++i)
			idx[i] = i;
	}	

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(int), idx, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if ( !hasTriangles )
		delete[] idx;
}


void Clouds::DrawPoints(float* projMtx, float* viewMtx, float pointSize, float useWhiteColor)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	uintptr_t offset = 0;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);

	// is has normal, colored with normal
	if (stride > 16 && !hasTriangles)
	{
		offset += sizeof(float)* 3 + sizeof(int);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	}
	else
	{
		offset += sizeof(float)* 3;
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offset);
	}	

	glUseProgram(prog);
	glUniformMatrix4fv(projMtxLoc, 1, GL_FALSE, projMtx);
	glUniformMatrix4fv(mvMtxLoc, 1, GL_FALSE, viewMtx);
	glUniform1f(sizeLoc, pointSize);
	glUniform1f(useWhiteLoc, useWhiteColor);
	
	glDrawElements(hasTriangles? GL_TRIANGLES : GL_POINTS, numIndices, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glUseProgram(0);
}

//////////////////////////////////////////////////////////////////////////
const char* BackgroundTex::vsCode =
"	#version 100 \n"
"	precision highp float;\n"
"	attribute vec3 a_position; \n"
"	attribute vec2 a_uv; \n"
"	varying vec2 uvCoord; \n"
"	\n"
"	void main()\n"
"	{\n"
"		gl_Position = vec4(a_position, 1.0);\n"
"		uvCoord = a_uv;\n"
"	}";

const char* BackgroundTex::fsCode =
"	#version 100 \n"
"	precision highp float;\n"
"	uniform sampler2D colorMap; \n"
"	varying vec2 uvCoord; \n"
"	\n"
"	void main()\n"
"	{\n"
"		gl_FragData[0] = texture2D(colorMap, uvCoord);\n"
"		//gl_FragData[0] = vec4(uvCoord,0,1);\n"
"	}";

BackgroundTex::BackgroundTex()
{
	glGenBuffers(1, &ibo);
	glGenBuffers(1, &vbo);
	prog = CreateShaderProgramFromString(vsCode, fsCode, "");
	texRegLoc = glGetUniformLocation(prog, "colorMap");

	float vert[4 * 5] = {
		-1, -1, 0.99f, 0, 1,
		 1, -1, 0.99f, 1, 1,
		 1,  1, 0.99f, 1, 0,
		-1,  1, 0.99f, 0, 0
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numVert * stride, vert, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	int idx[6] = { 0,1,2,0,2,3};
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(int), idx, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, 0);
	//printf("tex: %d, regLoc: %d\n", tex, texRegLoc);
}


BackgroundTex::~BackgroundTex()
{
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);
	glDeleteTextures(1, &tex);
}

void BackgroundTex::UploadTex(char* imgData, int w, int h)
{	
	glUseProgram(prog);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void BackgroundTex::Draw()
{
	glUseProgram(prog);
	glUniform1i(texRegLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	uintptr_t offset = 0;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	offset += sizeof(float)* 3;
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	
	glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(2);
	glUseProgram(0);
}

//////////////////////////////////////////////////////////////////////////

PointCloudViewer::PointCloudViewer(float focalLength)
{
	memset(projMtx, 0, sizeof(float)* 16);
	memset(viewMtx, 0, sizeof(float)* 16);

	if (!glfwInit())
		return;

	_win = glfwCreateWindow(800, 600, "3D Reconstruction", nullptr, nullptr);
	if (!_win)
	{
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(_win);

	GLenum glewResponse = glewInit();
	if (glewResponse != GLEW_OK)
		return;

	glfwSwapInterval(1);

	glClearColor(0.25, 0.25, 0.25, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	_SetPerspectiveLH(focalLength, VIEWER_W / VIEWER_H, 0.001f, 100.0f);
	//_SetPerspectiveRH(focalLength, VIEWER_W / VIEWER_H, 0.001f, 100.0f);
}

PointCloudViewer::~PointCloudViewer()
{
	RemoveAllClouds();
}

void PointCloudViewer::UpdateCamera(float *poseMtx, bool transpose)
{
	// poseMtx is row major....
	float eye[3], right[3], up[3], fwd[3];
	
	// for left hand
	if (transpose)
	{
		right[0] = poseMtx[0]; right[1] = poseMtx[4]; right[2] = poseMtx[8];
		up[0] = poseMtx[1]; up[1] = poseMtx[5]; up[2] = poseMtx[9];
		fwd[0] = poseMtx[2]; fwd[1] = poseMtx[6]; fwd[2] = poseMtx[10];
		eye[0] = poseMtx[3]; eye[1] = poseMtx[7]; eye[2] = poseMtx[11];
	}
	else
	{
		right[0] = poseMtx[0]; right[1] = poseMtx[1]; right[2] = poseMtx[2];
		up[0] = poseMtx[4]; up[1] = poseMtx[5]; up[2] = poseMtx[6];
		fwd[0] = poseMtx[8]; fwd[1] = poseMtx[9]; fwd[2] = poseMtx[10];
		eye[0] = poseMtx[12]; eye[1] = poseMtx[13]; eye[2] = poseMtx[14];
	}	
	
	// view mtx is column major, (and transpose rotate) 
	// --> row major to column major + transpose = canel the effect 
	viewMtx[0] = right[0]; viewMtx[4] = right[1]; viewMtx[8] = right[2];
	viewMtx[1] = up[0];    viewMtx[5] = up[1];    viewMtx[9] = up[2];
	viewMtx[2] = fwd[0];   viewMtx[6] = fwd[1];   viewMtx[10] = fwd[2];
	viewMtx[3] = 0.0f;     viewMtx[7] = 0.0f;     viewMtx[11] = 0.0f;
	
	Normalize(right);
	Normalize(up);
	Normalize(fwd);

	// put the viewer eye back-ward a little
	//eye[0] -= fwd[0] * 0.0f;
	//eye[1] -= fwd[1] * 0.0f;
	//eye[2] -= fwd[2] * 0.0f;

	// translate = -eye.dot.right / up / fwd
	viewMtx[12] = -Dot(eye, right); 
	viewMtx[13] = -Dot(eye, up);
	viewMtx[14] = -Dot(eye, fwd);
	viewMtx[15] = 1.0f;
}

void PointCloudViewer::UpdateBackground(char* color, int w, int h)
{
	if (!bg)
		bg = new BackgroundTex();

	bg->UploadTex(color, w, h);
}

void PointCloudViewer::UpdateGeometry(float *in_pointsCloud, int in_numPoints, int vertStride, int sector_num, int *sector_id_list, int *sector_vert_num, int *sector_mesh_id_num, int *in_index, int in_numIdx, bool enable_sector)
{
	if (prev_enable_sector != enable_sector) 
	{
		RemoveAllClouds();
		prev_enable_sector = enable_sector;
	}

	if (enable_sector) 
	{
		if (in_numIdx > 0) 
		{
			for (int i = 0, vert_id = 0, idx_id = 0; i < sector_num; i++)
			{
				int sector_id = sector_id_list[i];

				std::map<int, Clouds*>::iterator it = clouds.find(sector_id);
				if (it != clouds.end()) {
					delete it->second;
					clouds.erase(it);
				}

				int vert_num = sector_vert_num[i];
				int idx_num = sector_mesh_id_num[i];

				Clouds *cloud = new Clouds;
				cloud->UploadData(&in_pointsCloud[vert_id * 4], vert_num, vertStride, 3, &in_index[idx_id], idx_num);
				clouds[sector_id] = cloud;

				vert_id += vert_num;
				idx_id += idx_num;
			}
		}
		else
		{
			//std::cout << "sector num = " << sector_num << std::endl;
			for (int i = 0, vert_id = 0; i < sector_num; i++)
			{
				int sector_id = sector_id_list[i];

				std::map<int, Clouds*>::iterator it = clouds.find(sector_id);
				if (it != clouds.end()) {
					delete it->second;
					clouds.erase(it);
				}

				int vert_num = sector_vert_num[i];

				Clouds *cloud = new Clouds;
				cloud->UploadData(&in_pointsCloud[vert_id * 4], vert_num, vertStride, 3, in_index, in_numIdx);
				clouds[sector_id] = cloud;

				vert_id += vert_num;
			}
		}
	}
	else 
	{
		if(clouds.count(0) == 0)
			clouds[0] = new Clouds;

		clouds[0]->UploadData(in_pointsCloud, in_numPoints, vertStride, 3, in_index, in_numIdx);
	}
}

void PointCloudViewer::Draw(float pointSize, float useWhiteColor)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (bg)
	{
		glDepthMask(GL_FALSE);
		bg->Draw();
	}
	
	//if (clouds)
	if (clouds.size() != 0)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDepthMask(GL_TRUE);
		
		//clouds->DrawPoints(projMtx, viewMtx, pointSize, useWhiteColor);
		
		for (auto& cloud : clouds) {
			if (cloud.second != nullptr)
				cloud.second->DrawPoints(projMtx, viewMtx, pointSize, useWhiteColor);
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glfwSwapBuffers(_win);
	glfwPollEvents();
}

void PointCloudViewer::_SetPerspectiveLH(float focalLength, float aspect, float nearPlane, float farPlane)
{
	float fovRad = 2 * atanf(VIEWER_H / (2.0f * focalLength));

	float tanHalfFovy = tan(fovRad * 0.5f);
	projMtx[0] = 1.0f / (aspect * tanHalfFovy);
	projMtx[5] = 1.0f / (tanHalfFovy);
	projMtx[10] = farPlane / (farPlane - nearPlane);
	projMtx[11] = 1.0f;
	projMtx[14] = -(farPlane * nearPlane) / (farPlane - nearPlane);
	projMtx[15] = 0.0f;
}

void PointCloudViewer::_SetPerspectiveRH(float focalLength, float aspect, float nearPlane, float farPlane)
{
	float fovRad = 2 * atanf(VIEWER_H / (2.0f * focalLength));

	float tanHalfFovy = tan(fovRad * 0.5f);
	projMtx[0] = 1.0f / (aspect * tanHalfFovy);
	projMtx[5] = 1.0f / (tanHalfFovy);
	projMtx[10] = farPlane / (farPlane - nearPlane);
	projMtx[11] = -1.0f;
	projMtx[14] = (farPlane * nearPlane) / (farPlane - nearPlane);
	projMtx[15] = 0.0f;
}

//void PointCloudViewer::_SetPerspectiveLH(float fovDeg, float aspect, float nearPlane, float farPlane)
//{	
//	const float DegToRad = 3.14159265f / 180.0f;
//
//	float tanHalfFovy = tan(fovDeg * DegToRad * 0.5f);
//	projMtx[0] = 1.0f / (aspect * tanHalfFovy);
//	projMtx[5] = 1.0f / (tanHalfFovy);
//	projMtx[10] = farPlane / (farPlane - nearPlane);
//	projMtx[11] = 1.0f;
//	projMtx[14] = -(farPlane * nearPlane) / (farPlane - nearPlane);
//	projMtx[15] = 0.0f;
//}
//
//void PointCloudViewer::_SetPerspectiveRH(float fovDeg, float aspect, float nearPlane, float farPlane)
//{
//	const float DegToRad = 3.14159265f / 180.0f;
//
//	float tanHalfFovy = tan(fovDeg * DegToRad * 0.5f);
//	projMtx[0] = 1.0f / (aspect * tanHalfFovy);
//	projMtx[5] = 1.0f / (tanHalfFovy);
//	projMtx[10] = farPlane / (farPlane - nearPlane);
//	projMtx[11] = -1.0f;
//	projMtx[14] = (farPlane * nearPlane) / (farPlane - nearPlane);
//	projMtx[15] = 0.0f;
//}

void PointCloudViewer::RemoveAllClouds()
{
	for (auto& cloud : clouds)
		delete cloud.second;
	clouds.clear();
}