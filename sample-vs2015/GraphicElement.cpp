#include "headers.h"

const char* DepthColorTex::vsCode =
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

const char* DepthColorTex::fsCode =
"	#version 100 \n"
"	precision highp float;\n"
"	uniform sampler2D colorMap; \n"
"	varying vec2 uvCoord; \n"
"	\n"
"	void main()\n"
"	{\n"
"		float depth = 1.0 - texture2D(colorMap, uvCoord).r;\n"
"		vec3 color = vec3(1,1,1);\n"
"		if (depth < 0.25)\n"
"		{\n"
"			color.r = 0.0;\n"
"			color.g = 4.0 * depth;\n"
"		}\n"
"		else if (depth < 0.5)\n"
"		{\n"
"			color.r = 0.0;\n"
"			color.b = 1.0 + 4.0 * ( 0.25 - depth );\n"
"		}\n"
"		else if (depth < 0.75)\n"
"		{	\n"
"			color.b = 0.0;\n"
"			color.r = 4.0 * (depth - 0.5);\n"
"		}	\n"
"		else\n"
"		{	\n"
"			color.b = 0.0;\n"
"			color.g = 1.0 + 4.0 * (0.75 - depth);\n"
"		}	\n"
"		gl_FragData[0] = vec4(color,1);\n"
"		//gl_FragData[0] = vec4(uvCoord,0,1);\n"
"	}";

DepthColorTex::DepthColorTex(int width, int height)
{
	glGenBuffers(1, &_ibo);
	glGenBuffers(1, &_vbo);

	_prog = new pangolin::GlSlProgram();
	_prog->AddShader(pangolin::GlSlVertexShader, vsCode);
	_prog->AddShader(pangolin::GlSlFragmentShader, fsCode);
	_prog->BindPangolinDefaultAttribLocationsAndLink();
	_texRegLoc = _prog->GetUniformHandle("colorMap");

	float vert[4 * 5] = {
		-1, -1, 0.99f, 0, 1,
		1, -1, 0.99f, 1, 1,
		1,  1, 0.99f, 1, 0,
		-1,  1, 0.99f, 0, 0
	};

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, numVert * stride, vert, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	int idx[6] = { 0,1,2,0,2,3 };
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(int), idx, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	_depthTex.Reinitialise(width, height, GL_LUMINANCE, false, 0, GL_RED, GL_FLOAT);
}


DepthColorTex::~DepthColorTex()
{
	_depthTex.Delete();
	delete _prog;
	_prog = nullptr;
	glDeleteBuffers(1, &_vbo);
	glDeleteBuffers(1, &_ibo);	
}

void DepthColorTex::UploadTex(void* imgData, int w, int h)
{
	_depthTex.Upload(imgData, GL_RED, GL_FLOAT);
}


void DepthColorTex::Draw()
{
	_prog->Bind();

	glUniform1i(_texRegLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	_depthTex.Bind();

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);

	uintptr_t offset = 0;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	offset += sizeof(float) * 3;
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);

	glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	_depthTex.Unbind();
	_prog->Unbind();
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
"		gl_FragData[0] = color * vec4(1.0, 1.0, 1.0, 1.0);\n"
"	}";

Clouds::Clouds()
{
	_prog = new pangolin::GlSlProgram();
	_prog->AddShader(pangolin::GlSlVertexShader, vsCode);
	_prog->AddShader(pangolin::GlSlFragmentShader, fsCode);
	_prog->BindPangolinDefaultAttribLocationsAndLink();
	_projMtxLoc = _prog->GetUniformHandle("projMtx");
	_mvMtxLoc = _prog->GetUniformHandle("mvMtx");
	_sizeLoc = _prog->GetUniformHandle("pSizeScale");
	_useWhiteLoc = _prog->GetUniformHandle("useWhite");

	glGenBuffers(1, &_ibo);
	glGenBuffers(1, &_vbo);
}

Clouds::~Clouds()
{
	delete _prog;
	_prog = nullptr;
	glDeleteBuffers(1, &_vbo);
	glDeleteBuffers(1, &_ibo);
}

void Clouds::UpdateData(float* cloud, int cloud_size, int data_stride, int color_offset, int *in_index, int in_numIdx, GLuint priType)
{
	_numPoints = cloud_size;
	_offset = color_offset;
	_stride = data_stride;

	_numIndices = in_numIdx;
	_hasTriangles = true;
	_primitiveType = priType;
	int* cpuIdxData = in_index;		// temp

	if (_numIndices == 0)
	{
		_hasTriangles = false;
		_numIndices = _numPoints;
		cpuIdxData = new int[_numIndices];
		for (int i = 0; i < _numIndices; ++i)
			cpuIdxData[i] = i;

		_primitiveType = GL_POINTS;
	}

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, _numPoints * _stride, cloud, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _numIndices * sizeof(int), cpuIdxData, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (!_hasTriangles)
	{
		delete[] cpuIdxData;
	}
}

void Clouds::DrawPoints(float* projMtx, float* viewMtx, float pointSize, float useWhiteColor)
{
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);

	uintptr_t offset = 0;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, _stride, (void*)offset);

	// is has normal, colored with normal
	if (_stride > 16 && !_hasTriangles)
	{
		offset += sizeof(float) * 3 + sizeof(int);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, _stride, (void*)offset);
	}
	else
	{
		offset += sizeof(float) * 3;
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, _stride, (void*)offset);
	}

	_prog->Bind();
	//glUseProgram(prog);
	glUniformMatrix4fv(_projMtxLoc, 1, GL_FALSE, projMtx);
	glUniformMatrix4fv(_mvMtxLoc, 1, GL_FALSE, viewMtx);
	glUniform1f(_sizeLoc, pointSize);
	glUniform1f(_useWhiteLoc, useWhiteColor);

	glDrawElements(_primitiveType, _numIndices, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	//glUseProgram(0);
	_prog->Unbind();
}