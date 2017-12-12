#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		 
#include <GL/freeglut.h>	
#endif

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
const unsigned int windowWidth = 512, windowHeight = 512;

int majorVersion = 3, minorVersion = 0;

bool keyboardState[256];

void getErrorInfo(unsigned int handle)
{
	int logLen;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0)
	{
		char * log = new char[logLen];
		int written;
		glGetShaderInfoLog(handle, logLen, &written, log);
		printf("Shader log:\n%s", log);
		delete log;
	}
}

void checkShader(unsigned int shader, char * message)
{
	int OK;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
	if (!OK)
	{
		printf("%s!\n", message);
		getErrorInfo(shader);
	}
}

void checkLinking(unsigned int program)
{
	int OK;
	glGetProgramiv(program, GL_LINK_STATUS, &OK);
	if (!OK)
	{
		printf("Failed to link shader program!\n");
		getErrorInfo(program);
	}
}

// row-major matrix 4x4
struct mat4
{
	float m[4][4];
public:
	mat4() {}
	mat4(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33)
	{
		m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
		m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
		m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
		m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
	}

	mat4 operator*(const mat4& right)
	{
		mat4 result;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.m[i][j] = 0;
				for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
			}
		}
		return result;
	}
	operator float*() { return &m[0][0]; }
};


// 3D point in homogeneous coordinates
struct vec4
{
	float v[4];

	vec4(float x = 0, float y = 0, float z = 0, float w = 1)
	{
		v[0] = x; v[1] = y; v[2] = z; v[3] = w;
	}

	vec4 operator*(const mat4& mat)
	{
		vec4 result;
		for (int j = 0; j < 4; j++)
		{
			result.v[j] = 0;
			for (int i = 0; i < 4; i++) result.v[j] += v[i] * mat.m[i][j];
		}
		return result;
	}

	vec4 operator+(const vec4& vec)
	{
		vec4 result(v[0] + vec.v[0], v[1] + vec.v[1], v[2] + vec.v[2], v[3] + vec.v[3]);
		return result;
	}
};

// 2D point in Cartesian coordinates
struct vec2
{
	float x, y;

	vec2(float x = 0.0, float y = 0.0) : x(x), y(y) {}

	vec2 operator+(const vec2& v)
	{
		return vec2(x + v.x, y + v.y);
	}

	vec2 operator*(float s)
	{
		return vec2(x * s, y * s);
	}

};

// 3D point in Cartesian coordinates
struct vec3
{
	float x, y, z;
	float v[3];

	vec3(float x = 0.0, float y = 0.0, float z = 0.0) : x(x), y(y), z(z) {
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}

	static vec3 random() { return vec3(((float)rand() / RAND_MAX) * 2 - 1, ((float)rand() / RAND_MAX) * 2 - 1, ((float)rand() / RAND_MAX) * 2 - 1); }

	vec3 operator+(const vec3& v) { return vec3(x + v.x, y + v.y, z + v.z); }

	vec3 operator-(const vec3& v) { return vec3(x - v.x, y - v.y, z - v.z); }

	vec3 operator*(float s) { return vec3(x * s, y * s, z * s); }

	vec3 operator/(float s) { return vec3(x / s, y / s, z / s); }

	float length() { return sqrt(x * x + y * y + z * z); }

	vec3 normalize() { return *this / length(); }

	void print() { printf("%f \t %f \t %f \n", x, y, z); }
};

vec3 cross(const vec3& a, const vec3& b)
{
	return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}



class Geometry
{
protected:
	unsigned int vao;

public:
	Geometry()
	{
		glGenVertexArrays(1, &vao);
	}

	virtual void Draw() = 0;
};

class TexturedQuad : public Geometry {
	unsigned int vbos[3];

public:
	TexturedQuad() {
		glBindVertexArray(vao);

		glGenBuffers(3, &vbos[0]);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
		static float vertexCoords[] = {-1,0,-1, 
			-1, 0, 1, 
			1, 0, -1,
			1, 0, 1};
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
		static float texCoords[] = {0, 0, 1, 0, 0, 1, 1, 1};
		glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
		static float normVecs[] = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 };
		glBufferData(GL_ARRAY_BUFFER, sizeof(normVecs), normVecs, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	void Draw() {
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
	}
};

class InfiniteTexturedQuad : public Geometry {
	unsigned int vbos[3];

public:
	InfiniteTexturedQuad() {
		glBindVertexArray(vao);

		glGenBuffers(3, &vbos[0]);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
		static float vertexCoords[] = {0, 0, 0, 1,
			-1,0,-1, 0,
			-1, 0, 1, 0,
			1, 0, 1, 0,
			1, 0, -1, 0,
			-1,0,-1, 0
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
		static float texCoords[] = { 0, 0, 1, 0, 0, 1, 1, 1 };
		glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
		static float normVecs[] = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 };
		glBufferData(GL_ARRAY_BUFFER, sizeof(normVecs), normVecs, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	void Draw() {
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
	}
};


class   PolygonalMesh : public Geometry
{
	struct  Face
	{
		int       positionIndices[4];
		int       normalIndices[4];
		int       texcoordIndices[4];
		bool      isQuad;
	};

	std::vector<std::string*> rows;
	std::vector<vec3*> positions;
	std::vector<std::vector<Face*>> submeshFaces;
	std::vector<vec3*> normals;
	std::vector<vec2*> texcoords;

	int nTriangles;

public:
	PolygonalMesh(const char *filename);
	~PolygonalMesh();

	void Draw();
};



PolygonalMesh::PolygonalMesh(const char *filename)
{
	std::fstream file(filename);
	if (!file.is_open())
	{
		return;
	}

	char buffer[256];
	while (!file.eof())
	{
		file.getline(buffer, 256);
		rows.push_back(new std::string(buffer));
	}

	submeshFaces.push_back(std::vector<Face*>());
	std::vector<Face*>* faces = &submeshFaces.at(submeshFaces.size() - 1);

	for (int i = 0; i < rows.size(); i++)
	{
		if (rows[i]->empty() || (*rows[i])[0] == '#')
			continue;
		else if ((*rows[i])[0] == 'v' && (*rows[i])[1] == ' ')
		{
			float tmpx, tmpy, tmpz;
			sscanf(rows[i]->c_str(), "v %f %f %f", &tmpx, &tmpy, &tmpz);
			positions.push_back(new vec3(tmpx, tmpy, tmpz));
		}
		else if ((*rows[i])[0] == 'v' && (*rows[i])[1] == 'n')
		{
			float tmpx, tmpy, tmpz;
			sscanf(rows[i]->c_str(), "vn %f %f %f", &tmpx, &tmpy, &tmpz);
			normals.push_back(new vec3(tmpx, tmpy, tmpz));
		}
		else if ((*rows[i])[0] == 'v' && (*rows[i])[1] == 't')
		{
			float tmpx, tmpy;
			sscanf(rows[i]->c_str(), "vt %f %f", &tmpx, &tmpy);
			texcoords.push_back(new vec2(tmpx, tmpy));
		}
		else if ((*rows[i])[0] == 'f')
		{
			if (count(rows[i]->begin(), rows[i]->end(), ' ') == 3)
			{
				Face* f = new Face();
				f->isQuad = false;
				sscanf(rows[i]->c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d",
					&f->positionIndices[0], &f->texcoordIndices[0], &f->normalIndices[0],
					&f->positionIndices[1], &f->texcoordIndices[1], &f->normalIndices[1],
					&f->positionIndices[2], &f->texcoordIndices[2], &f->normalIndices[2]);
				faces->push_back(f);
			}
			else
			{
				Face* f = new Face();
				f->isQuad = true;
				sscanf(rows[i]->c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
					&f->positionIndices[0], &f->texcoordIndices[0], &f->normalIndices[0],
					&f->positionIndices[1], &f->texcoordIndices[1], &f->normalIndices[1],
					&f->positionIndices[2], &f->texcoordIndices[2], &f->normalIndices[2],
					&f->positionIndices[3], &f->texcoordIndices[3], &f->normalIndices[3]);
				faces->push_back(f);
			}
		}
		else if ((*rows[i])[0] == 'g')
		{
			if (faces->size() > 0)
			{
				submeshFaces.push_back(std::vector<Face*>());
				faces = &submeshFaces.at(submeshFaces.size() - 1);
			}
		}
	}

	int numberOfTriangles = 0;
	for (int iSubmesh = 0; iSubmesh<submeshFaces.size(); iSubmesh++)
	{
		std::vector<Face*>& faces = submeshFaces.at(iSubmesh);

		for (int i = 0;i<faces.size();i++)
		{
			if (faces[i]->isQuad) numberOfTriangles += 2;
			else numberOfTriangles += 1;
		}
	}

	nTriangles = numberOfTriangles;

	float *vertexCoords = new float[numberOfTriangles * 9];
	float *vertexTexCoords = new float[numberOfTriangles * 6];
	float *vertexNormalCoords = new float[numberOfTriangles * 9];


	int triangleIndex = 0;
	for (int iSubmesh = 0; iSubmesh<submeshFaces.size(); iSubmesh++)
	{
		std::vector<Face*>& faces = submeshFaces.at(iSubmesh);

		for (int i = 0;i<faces.size();i++)
		{
			if (faces[i]->isQuad)
			{
				vertexTexCoords[triangleIndex * 6] = texcoords[faces[i]->texcoordIndices[0] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 1] = 1 - texcoords[faces[i]->texcoordIndices[0] - 1]->y;

				vertexTexCoords[triangleIndex * 6 + 2] = texcoords[faces[i]->texcoordIndices[1] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 3] = 1 - texcoords[faces[i]->texcoordIndices[1] - 1]->y;

				vertexTexCoords[triangleIndex * 6 + 4] = texcoords[faces[i]->texcoordIndices[2] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 5] = 1 - texcoords[faces[i]->texcoordIndices[2] - 1]->y;


				vertexCoords[triangleIndex * 9] = positions[faces[i]->positionIndices[0] - 1]->x;
				vertexCoords[triangleIndex * 9 + 1] = positions[faces[i]->positionIndices[0] - 1]->y;
				vertexCoords[triangleIndex * 9 + 2] = positions[faces[i]->positionIndices[0] - 1]->z;

				vertexCoords[triangleIndex * 9 + 3] = positions[faces[i]->positionIndices[1] - 1]->x;
				vertexCoords[triangleIndex * 9 + 4] = positions[faces[i]->positionIndices[1] - 1]->y;
				vertexCoords[triangleIndex * 9 + 5] = positions[faces[i]->positionIndices[1] - 1]->z;

				vertexCoords[triangleIndex * 9 + 6] = positions[faces[i]->positionIndices[2] - 1]->x;
				vertexCoords[triangleIndex * 9 + 7] = positions[faces[i]->positionIndices[2] - 1]->y;
				vertexCoords[triangleIndex * 9 + 8] = positions[faces[i]->positionIndices[2] - 1]->z;


				vertexNormalCoords[triangleIndex * 9] = normals[faces[i]->normalIndices[0] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 1] = normals[faces[i]->normalIndices[0] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 2] = normals[faces[i]->normalIndices[0] - 1]->z;

				vertexNormalCoords[triangleIndex * 9 + 3] = normals[faces[i]->normalIndices[1] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 4] = normals[faces[i]->normalIndices[1] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 5] = normals[faces[i]->normalIndices[1] - 1]->z;

				vertexNormalCoords[triangleIndex * 9 + 6] = normals[faces[i]->normalIndices[2] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 7] = normals[faces[i]->normalIndices[2] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 8] = normals[faces[i]->normalIndices[2] - 1]->z;

				triangleIndex++;


				vertexTexCoords[triangleIndex * 6] = texcoords[faces[i]->texcoordIndices[1] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 1] = 1 - texcoords[faces[i]->texcoordIndices[1] - 1]->y;

				vertexTexCoords[triangleIndex * 6 + 2] = texcoords[faces[i]->texcoordIndices[2] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 3] = 1 - texcoords[faces[i]->texcoordIndices[2] - 1]->y;

				vertexTexCoords[triangleIndex * 6 + 4] = texcoords[faces[i]->texcoordIndices[3] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 5] = 1 - texcoords[faces[i]->texcoordIndices[3] - 1]->y;


				vertexCoords[triangleIndex * 9] = positions[faces[i]->positionIndices[1] - 1]->x;
				vertexCoords[triangleIndex * 9 + 1] = positions[faces[i]->positionIndices[1] - 1]->y;
				vertexCoords[triangleIndex * 9 + 2] = positions[faces[i]->positionIndices[1] - 1]->z;

				vertexCoords[triangleIndex * 9 + 3] = positions[faces[i]->positionIndices[2] - 1]->x;
				vertexCoords[triangleIndex * 9 + 4] = positions[faces[i]->positionIndices[2] - 1]->y;
				vertexCoords[triangleIndex * 9 + 5] = positions[faces[i]->positionIndices[2] - 1]->z;

				vertexCoords[triangleIndex * 9 + 6] = positions[faces[i]->positionIndices[3] - 1]->x;
				vertexCoords[triangleIndex * 9 + 7] = positions[faces[i]->positionIndices[3] - 1]->y;
				vertexCoords[triangleIndex * 9 + 8] = positions[faces[i]->positionIndices[3] - 1]->z;


				vertexNormalCoords[triangleIndex * 9] = normals[faces[i]->normalIndices[1] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 1] = normals[faces[i]->normalIndices[1] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 2] = normals[faces[i]->normalIndices[1] - 1]->z;

				vertexNormalCoords[triangleIndex * 9 + 3] = normals[faces[i]->normalIndices[2] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 4] = normals[faces[i]->normalIndices[2] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 5] = normals[faces[i]->normalIndices[2] - 1]->z;

				vertexNormalCoords[triangleIndex * 9 + 6] = normals[faces[i]->normalIndices[3] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 7] = normals[faces[i]->normalIndices[3] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 8] = normals[faces[i]->normalIndices[3] - 1]->z;

				triangleIndex++;
			}
			else
			{
				vertexTexCoords[triangleIndex * 6] = texcoords[faces[i]->texcoordIndices[0] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 1] = 1 - texcoords[faces[i]->texcoordIndices[0] - 1]->y;

				vertexTexCoords[triangleIndex * 6 + 2] = texcoords[faces[i]->texcoordIndices[1] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 3] = 1 - texcoords[faces[i]->texcoordIndices[1] - 1]->y;

				vertexTexCoords[triangleIndex * 6 + 4] = texcoords[faces[i]->texcoordIndices[2] - 1]->x;
				vertexTexCoords[triangleIndex * 6 + 5] = 1 - texcoords[faces[i]->texcoordIndices[2] - 1]->y;

				vertexCoords[triangleIndex * 9] = positions[faces[i]->positionIndices[0] - 1]->x;
				vertexCoords[triangleIndex * 9 + 1] = positions[faces[i]->positionIndices[0] - 1]->y;
				vertexCoords[triangleIndex * 9 + 2] = positions[faces[i]->positionIndices[0] - 1]->z;

				vertexCoords[triangleIndex * 9 + 3] = positions[faces[i]->positionIndices[1] - 1]->x;
				vertexCoords[triangleIndex * 9 + 4] = positions[faces[i]->positionIndices[1] - 1]->y;
				vertexCoords[triangleIndex * 9 + 5] = positions[faces[i]->positionIndices[1] - 1]->z;

				vertexCoords[triangleIndex * 9 + 6] = positions[faces[i]->positionIndices[2] - 1]->x;
				vertexCoords[triangleIndex * 9 + 7] = positions[faces[i]->positionIndices[2] - 1]->y;
				vertexCoords[triangleIndex * 9 + 8] = positions[faces[i]->positionIndices[2] - 1]->z;


				vertexNormalCoords[triangleIndex * 9] = normals[faces[i]->normalIndices[0] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 1] = normals[faces[i]->normalIndices[0] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 2] = normals[faces[i]->normalIndices[0] - 1]->z;

				vertexNormalCoords[triangleIndex * 9 + 3] = normals[faces[i]->normalIndices[1] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 4] = normals[faces[i]->normalIndices[1] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 5] = normals[faces[i]->normalIndices[1] - 1]->z;

				vertexNormalCoords[triangleIndex * 9 + 6] = normals[faces[i]->normalIndices[2] - 1]->x;
				vertexNormalCoords[triangleIndex * 9 + 7] = normals[faces[i]->normalIndices[2] - 1]->y;
				vertexNormalCoords[triangleIndex * 9 + 8] = normals[faces[i]->normalIndices[2] - 1]->z;

				triangleIndex++;
			}
		}
	}

	glBindVertexArray(vao);

	unsigned int vbo[3];
	glGenBuffers(3, &vbo[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, nTriangles * 9 * sizeof(float), vertexCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, nTriangles * 6 * sizeof(float), vertexTexCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, nTriangles * 9 * sizeof(float), vertexNormalCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	delete vertexCoords;
	delete vertexTexCoords;
	delete vertexNormalCoords;
}


void PolygonalMesh::Draw()
{
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, nTriangles * 3);
	glDisable(GL_DEPTH_TEST);
}


PolygonalMesh::~PolygonalMesh()
{
	for (unsigned int i = 0; i < rows.size(); i++) delete rows[i];
	for (unsigned int i = 0; i < positions.size(); i++) delete positions[i];
	for (unsigned int i = 0; i < submeshFaces.size(); i++)
		for (unsigned int j = 0; j < submeshFaces.at(i).size(); j++)
			delete submeshFaces.at(i).at(j);
	for (unsigned int i = 0; i < normals.size(); i++) delete normals[i];
	for (unsigned int i = 0; i < texcoords.size(); i++) delete texcoords[i];
}



class Shader
{
protected:
	unsigned int shaderProgram;

public:
	Shader()
	{
		shaderProgram = 0;
	}

	~Shader()
	{
		if (shaderProgram) glDeleteProgram(shaderProgram);
	}

	void Run()
	{
		if (shaderProgram) glUseProgram(shaderProgram);
	}

	virtual void UploadInvM(mat4& InVM) { }

	virtual void UploadMVP(mat4& MVP) { }
	virtual void UploadVP(mat4& VP) {}

	virtual void UploadM(mat4& M) {}

	virtual void UploadSamplerID() { }

	virtual void UploadMaterialAttributes(vec3 ka, vec3 kd, vec3 ks, float shininess) {}

	virtual void UploadLightAttributes(vec3 La, vec3 Le, vec4 worldLightPosition) {}

	virtual void UploadEyePosition(vec3 wEye) {}
};

class ShadowShader : public Shader {
public:
	ShadowShader() {
		const char *vertexSource = " \n\
			#version 130 \n\
			precision highp float; \n\
			\n\
			in vec3 vertexPosition; \n\
			in vec2 vertexTexCoord; \n\
			in vec3 vertexNormal; \n\
			uniform mat4 M, VP; \n\
			uniform vec4 worldLightPosition; \n\
			\n\
			void main() { \n\
			vec4 p = vec4(vertexPosition, 1) * M; \n\
			vec3 s; \n\
			s.y = -0.999; \n\
			s.x = (p.x - worldLightPosition.x) / (p.y - worldLightPosition.y) * (s.y - worldLightPosition.y) + worldLightPosition.x; \n\
			s.z = (p.z - worldLightPosition.z) / (p.y - worldLightPosition.y) * (s.y - worldLightPosition.y) + worldLightPosition.z; \n\
			gl_Position = vec4(s, 1) * VP; \n\
			} \n\
		";

		const char *fragmentSource = " \n\
			#version 130 \n\
			precision highp float; \n\
			\n\
			out vec4 fragmentColor; \n\
			\n\
			void main() { \n\
			fragmentColor = vec4(0.0, 0.1, 0.0, 1); \n\
			} \n\
		";

		unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }

		glShaderSource(vertexShader, 1, &vertexSource, NULL);
		glCompileShader(vertexShader);
		checkShader(vertexShader, "Vertex shader error");

		unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }

		glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
		glCompileShader(fragmentShader);
		checkShader(fragmentShader, "Fragment shader error");

		shaderProgram = glCreateProgram();
		if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }

		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);

		glBindAttribLocation(shaderProgram, 0, "vertexPosition");
		glBindAttribLocation(shaderProgram, 1, "vertexTexCoord");
		glBindAttribLocation(shaderProgram, 2, "vertexNormal");

		glBindFragDataLocation(shaderProgram, 0, "fragmentColor");

		glLinkProgram(shaderProgram);
		checkLinking(shaderProgram);
	}

	void UploadM(mat4& M) {
		int location = glGetUniformLocation(shaderProgram, "M");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
		else printf("uniform M cannot be set\n");
	}

	void UploadVP(mat4& VP)
	{
		int location = glGetUniformLocation(shaderProgram, "VP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, VP);
		else printf("uniform VP cannot be set\n");
	}

	void UploadLightAttributes(vec3 La, vec3 Le, vec4 worldLightPosition) {
		int location = glGetUniformLocation(shaderProgram, "worldLightPosition");
		if (location >= 0) glUniform4fv(location, 1, &worldLightPosition.v[0]);
		else printf("uniform worldLightPosition cannot be set\n");
	}
};

class InfiniteQuadShader : public Shader {
public:
	InfiniteQuadShader() {
		const char *vertexSource = "\n\
			#version 130 \n\
			precision highp float; \n\
			\n\
			in vec4 vertexPosition; \n\
			in vec2 vertexTexCoord; \n\
			in vec3 vertexNormal; \n\
			uniform mat4 M, InvM, MVP; \n\
			\n\
			out vec2 texCoord; \n\
			out vec4 worldPosition; \n\
			out vec3 worldNormal; \n\
			\n\
			void main() { \n\
			texCoord = vertexTexCoord; \n\
			worldPosition = vertexPosition * M; \n\
			worldNormal = (InvM * vec4(vertexNormal, 0.0)).xyz; \n\
			gl_Position = vertexPosition * MVP; \n\
			} \n\
		";

		const char *fragmentSource = "\n\
			#version 130 \n\
			precision highp float; \n\
			uniform sampler2D samplerUnit; \n\
			uniform vec3 La, Le; \n\
			uniform vec3 ka, kd, ks; \n\
			uniform float shininess; \n\
			uniform vec3 worldEyePosition; \n\
			uniform vec4 worldLightPosition; \n\
			in vec2 texCoord; \n\
			in vec4 worldPosition; \n\
			in vec3 worldNormal; \n\
			out vec4 fragmentColor; \n\
			void main() { \n\
			vec3 N = normalize(worldNormal); \n\
			vec3 V = normalize(worldEyePosition * worldPosition.w - worldPosition.xyz);\n\
			vec3 L = normalize(worldLightPosition.xyz * worldPosition.w - worldPosition.xyz * worldLightPosition.w);\n\
			vec3 H = normalize(V + L); \n\
			vec2 position = worldPosition.xz / worldPosition.w; \n\
			vec2 tex = position.xy - floor(position.xy); \n\
			vec3 texel = texture(samplerUnit, tex).xyz; \n\
			vec3 color = La * ka + Le * kd * texel* max(0.0, dot(L, N)) + Le * ks * pow(max(0.0, dot(H, N)), shininess); \n\
			fragmentColor = vec4(color, 1); \n\
			} \n\
		";

		unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }

		glShaderSource(vertexShader, 1, &vertexSource, NULL);
		glCompileShader(vertexShader);
		checkShader(vertexShader, "Vertex shader error");

		unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }

		glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
		glCompileShader(fragmentShader);
		checkShader(fragmentShader, "Fragment shader error");

		shaderProgram = glCreateProgram();
		if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }

		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);

		glBindAttribLocation(shaderProgram, 0, "vertexPosition");
		glBindAttribLocation(shaderProgram, 1, "vertexTexCoord");
		glBindAttribLocation(shaderProgram, 2, "vertexNormal");

		glBindFragDataLocation(shaderProgram, 0, "fragmentColor");

		glLinkProgram(shaderProgram);
		checkLinking(shaderProgram);
	}

	void UploadMaterialAttributes(vec3 ka, vec3 kd, vec3 ks, float shininess) {
		int location = glGetUniformLocation(shaderProgram, "ka");
		if (location >= 0) glUniform3f(location, ka.x, ka.y, ka.z);
		else printf("uniform ka cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "kd");
		if (location >= 0) glUniform3f(location, kd.x, kd.y, kd.z);
		else printf("uniform kd cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "ks");
		if (location >= 0) glUniform3f(location, ks.x, ks.y, ks.z);
		else printf("uniform ks cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "shininess");
		if (location >= 0) glUniform1f(location, shininess);
		else printf("uniform shininess cannot be set\n");
	}

	void UploadEyePosition(vec3 wEye) {
		int location = glGetUniformLocation(shaderProgram, "worldEyePosition");
		if (location >= 0) glUniform3f(location, wEye.x, wEye.y, wEye.z);
		else printf("uniform worldEyePosition cannot be set");
	}

	void UploadLightAttributes(vec3 La, vec3 Le, vec4 worldLightPosition) {
		int location = glGetUniformLocation(shaderProgram, "La");
		if (location >= 0) glUniform3f(location, La.x, La.y, La.z);
		else printf("uniform La cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "Le");
		if (location >= 0) glUniform3f(location, Le.x, Le.y, Le.z);
		else printf("uniform Le cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "worldLightPosition");
		if (location >= 0) glUniform4fv(location, 1, &worldLightPosition.v[0]);
		else printf("uniform worldLightPosition cannot be set\n");
	}

	void UploadSamplerID()
	{
		int samplerUnit = 0;
		int location = glGetUniformLocation(shaderProgram, "samplerUnit");
		glUniform1i(location, samplerUnit);
		glActiveTexture(GL_TEXTURE0 + samplerUnit);
	}

	void UploadM(mat4& M) {
		int location = glGetUniformLocation(shaderProgram, "M");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
		else printf("uniform M cannot be set\n");
	}

	void UploadInvM(mat4& InvM)
	{
		int location = glGetUniformLocation(shaderProgram, "InvM");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, InvM);
		else printf("uniform InvM cannot be set\n");
	}

	void UploadMVP(mat4& MVP)
	{
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVP);
		else printf("uniform MVP cannot be set\n");
	}
};


class MeshShader : public Shader
{
public:
	MeshShader()
	{
		const char *vertexSource = "\n\
			#version 130 \n\
    		precision highp float; \n\
			\n\
			in vec3 vertexPosition; \n\
			in vec2 vertexTexCoord; \n\
			in vec3 vertexNormal; \n\
			uniform mat4 M, InvM, MVP; \n\
			uniform vec3 worldEyePosition;\n\
			uniform vec4 worldLightPosition;\n\
			out vec2 texCoord; \n\
			out vec3 worldNormal; \n\
			out vec3 worldView;\n\
			out vec3 worldLight;\n\
			\n\
			void main() { \n\
				texCoord = vertexTexCoord; \n\
				vec4 worldPosition = vec4(vertexPosition, 1) * M;\n\
				worldLight = worldLightPosition.xyz * worldPosition.w - worldPosition.xyz * worldLightPosition.w;\n\
				worldView = worldEyePosition - worldPosition.xyz;\n\
				worldNormal = (InvM * vec4(vertexNormal, 0.0)).xyz;\n\
				gl_Position = vec4(vertexPosition, 1) * MVP;\n\
			} \n\
		";

		const char *fragmentSource = "\n\
			#version 130 \n\
    		precision highp float; \n\
			\n\
			uniform sampler2D samplerUnit; \n\
			uniform vec3 La, Le;\n\
			uniform vec3 ka, kd, ks;\n\
			uniform float shininess;\n\
			in vec2 texCoord; \n\
			in vec3 worldNormal; \n\
			in vec3 worldView;\n\
			in vec3 worldLight;\n\
			out vec4 fragmentColor; \n\
			\n\
			void main() { \n\
				vec3 N = normalize(worldNormal);\n\
				vec3 V = normalize(worldView);\n\
				vec3 L = normalize(worldLight);\n\
				vec3 H = normalize(V + L);\n\
				vec3 texel = texture(samplerUnit, texCoord).xyz;\n\
				vec3 color = La * ka + Le * kd * texel * max(0.0, dot(L, N)) + Le * ks * pow(max(0.0, dot(H, N)), shininess);\n\
				fragmentColor = vec4(color, 1);\n\
			} \n\
		";

		unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }

		glShaderSource(vertexShader, 1, &vertexSource, NULL);
		glCompileShader(vertexShader);
		checkShader(vertexShader, "Vertex shader error");

		unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }

		glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
		glCompileShader(fragmentShader);
		checkShader(fragmentShader, "Fragment shader error");

		shaderProgram = glCreateProgram();
		if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }

		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);

		glBindAttribLocation(shaderProgram, 0, "vertexPosition");
		glBindAttribLocation(shaderProgram, 1, "vertexTexCoord");
		glBindAttribLocation(shaderProgram, 2, "vertexNormal");

		glBindFragDataLocation(shaderProgram, 0, "fragmentColor");

		glLinkProgram(shaderProgram);
		checkLinking(shaderProgram);
	}

	void UploadMaterialAttributes(vec3 ka, vec3 kd, vec3 ks, float shininess) {
		int location = glGetUniformLocation(shaderProgram, "ka");
		if (location >= 0) glUniform3f(location, ka.x, ka.y, ka.z);
		else printf("uniform ka cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "kd");
		if (location >= 0) glUniform3f(location, kd.x, kd.y, kd.z);
		else printf("uniform kd cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "ks");
		if (location >= 0) glUniform3f(location, ks.x, ks.y, ks.z);
		else printf("uniform ks cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "shininess");
		if (location >= 0) glUniform1f(location, shininess);
		else printf("uniform shininess cannot be set\n");
	}

	void UploadEyePosition(vec3 wEye) {
		int location = glGetUniformLocation(shaderProgram, "worldEyePosition");
		if (location >= 0) glUniform3f(location, wEye.x, wEye.y, wEye.z);
		else printf("uniform worldEyePosition cannot be set");
	}

	void UploadLightAttributes(vec3 La, vec3 Le, vec4 worldLightPosition) {
		int location = glGetUniformLocation(shaderProgram, "La");
		if (location >= 0) glUniform3f(location, La.x, La.y, La.z);
		else printf("uniform La cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "Le");
		if (location >= 0) glUniform3f(location, Le.x, Le.y, Le.z);
		else printf("uniform Le cannot be set\n");

		location = glGetUniformLocation(shaderProgram, "worldLightPosition");
		if (location >= 0) glUniform4fv(location, 1, &worldLightPosition.v[0]);
		else printf("uniform worldLightPosition cannot be set\n");
	}

	void UploadSamplerID()
	{
		int samplerUnit = 0;
		int location = glGetUniformLocation(shaderProgram, "samplerUnit");
		glUniform1i(location, samplerUnit);
		glActiveTexture(GL_TEXTURE0 + samplerUnit);
	}

	void UploadM(mat4& M) {
		int location = glGetUniformLocation(shaderProgram, "M");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
		else printf("uniform M cannot be set\n");
	}

	void UploadInvM(mat4& InvM)
	{
		int location = glGetUniformLocation(shaderProgram, "InvM");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, InvM);
		else printf("uniform InvM cannot be set\n");
	}

	void UploadMVP(mat4& MVP)
	{
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVP);
		else printf("uniform MVP cannot be set\n");
	}
};



extern "C" unsigned char* stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);

class Texture
{
	unsigned int textureId;

public:
	Texture(const std::string& inputFileName)
	{
		unsigned char* data;
		int width; int height; int nComponents = 4;

		data = stbi_load(inputFileName.c_str(), &width, &height, &nComponents, 0);

		if (data == NULL)
		{
			return;
		}

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		if (nComponents == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		if (nComponents == 4) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		delete data;
	}

	void Bind()
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
	}
};


class Light {
	vec3 La;
	vec3 Le;
	vec4 worldLightPosition;
	

public:
	Light() :
		La(1.0, 1.0, 1.0),
		Le(1.0, 1.0, 1.0),
		worldLightPosition(0, 0, 2.0, 1.0)
	{}

	Light(vec3 La, vec3 Le, vec4 worldLightPosition) {
		this->La = La;
		this->Le = Le;
		this->worldLightPosition = worldLightPosition;
	}

	void UploadAttributes(Shader* shader) {
		shader->UploadLightAttributes(La, Le, worldLightPosition);
	}

	void SetPointLightSource(vec3& pos) {
		worldLightPosition.v[0] = pos.x;
		worldLightPosition.v[1] = pos.y;
		worldLightPosition.v[2] = pos.z;
		worldLightPosition.v[3] = 1.0;
	}

	void SetDirectionalLightSource(vec3& dir) {
		worldLightPosition.v[0] = dir.x;
		worldLightPosition.v[1] = dir.y;
		worldLightPosition.v[2] = dir.z;
		worldLightPosition.v[3] = 0.0;
	}
};

class Material
{
	Shader* shader;
	Texture* texture;
	vec3 ka;
	vec3 kd;
	vec3 ks;
	float shininess;


public:
	Material(Shader* s, vec3 ka, vec3 kd, vec3 ks, float shininess, Texture* t = 0)
	{
		shader = s;
		texture = t;
		this->ka = ka;
		this->kd = kd;
		this->ks = ks;
		this->shininess = shininess;
	}

	Shader* GetShader() { return shader; }

	void UploadAttributes()
	{
		if (texture)
		{
			shader->UploadMaterialAttributes(ka, kd, ks, shininess);
			shader->UploadSamplerID();
			texture->Bind();
		}
	}
};

class Mesh
{
	Geometry* geometry;
	Material* material;

public:
	Mesh(Geometry* g, Material* m)
	{
		geometry = g;
		material = m;
	}

	Shader* GetShader() { return material->GetShader(); }

	void Draw()
	{
		material->UploadAttributes();
		geometry->Draw();
	}

	void UploadAttributes() {
		material->UploadAttributes();
	}
};

Light* light;
Light* spotlight;
vec3 initialPos;
vec3 initialHat;


class Camera {
	vec3  wEye, wLookat, wVup;
	float fov, asp, fp, bp;

	vec3 velocity;
	vec3 acc;
	float angularVelocity;

public:
	Camera()
	{
		wEye = vec3(0.0, -0.5, 2.5);
		wLookat = vec3(0.0, -0.5, 0.0);
		wVup = vec3(0.0, 1.0, 0.0);
		fov = M_PI / 4.0; asp = 1.0; fp = 0.01; bp = 10.0;
		velocity = 0; angularVelocity = 0;
	}

	void SetAcceleration() {
		vec3 dif = wLookat - wEye;

		if (keyboardState['w'] || keyboardState['s']) {
			acc = dif * 0.03;
		}
		else {
			if (velocity.length() > 0) {
				acc = velocity * -0.01;
			}

		}
	}

	void Control() {
		//the 2 speeds include directions
		float speed = 1;
		vec3 dif = wLookat - wEye;

		SetAcceleration();

		if (keyboardState['w'] || keyboardState['s']) {
			if ((velocity.x*dif.x + velocity.z*dif.z) < 5) {
				velocity = velocity + acc * (keyboardState['w'] * speed + keyboardState['s'] * -speed);
			}
		}
		else {
			velocity = velocity + acc * speed;
		}


		float angularSpeed = 1;

		angularVelocity = keyboardState['a'] * -angularSpeed + keyboardState['d'] * angularSpeed;
	}

	/*void Control() {
		float speed = 1;
		vec3 dif = wLookat - wEye;
		velocity = dif * (keyboardState['w'] * speed + keyboardState['s'] * -speed);

		float angularSpeed = 1;
		angularVelocity = keyboardState['a'] * -angularSpeed + keyboardState['d'] * angularSpeed;

		
	}*/

	vec3 GetLookAt()
	{
		return wLookat;
	}

	vec3 GetwEye()
	{
		return wEye;
	}

	void SetwEye(vec3 v)
	{
		wEye = v;
	}

	void setEyeX(float x) {
		wEye.x = x;
	}

	void setEyeZ(float z) {
		wEye.z = z;
	}


	void TrackingShot(float t)
	{
		wEye = heart(t);
	}

	void setVelocity(vec3 v) {
		velocity = v;
	}

	vec3 getVelocity() {
		return velocity;
	}

	vec3 heart(float t) {
		vec3 v0 = vec3(0, 0, -2).normalize();
		vec3 v1 = (wLookat - initialPos).normalize();
		float co = v0.z*v1.z; // dot product
		float si = cross(v0, v1).length(); // size of cross product

										   //        vec3 heartToEye = vec3(x(t),0,z(t)) - wEye;

		float heartToEyeX = x(t);
		float heartToEyeZ = z(t) - 2;

		vec3 eyeToPos = vec3(heartToEyeX*co - heartToEyeZ*si, 0, heartToEyeX*si + heartToEyeZ*co);

		return eyeToPos + initialPos;
	}


	//parametric function scaled down by 17/2
	float x(float t) {
		return 16 * sin(t)*sin(t)*sin(t) / 3;
	}

	float z(float t) {
		t += M_PI;
		return -(13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t)) / 3;
	}

	void Move(float dt) {
		wEye = wEye + velocity*dt;
		wLookat = wLookat + velocity * dt;

		vec3 w = (wLookat - wEye).normalize();
		vec3 v = wVup.normalize();
		vec3 u = cross(w, v);

		vec3 wPrime = (w*cos(angularVelocity*dt) + u * sin(angularVelocity*dt)) * (wLookat - wEye).length();
		//wLookat = wPrime + wEye;
		//wEye = wLookat - wPrime;

		if (keyboardState['t']) {
			wPrime = (w*cos(dt) + u * sin(dt)) * (wLookat - wEye).length();
		}
			wEye = wLookat - wPrime;
		

		

	}


	void UploadAttribtes(Shader* shader) {
		shader->UploadEyePosition(wEye);
	}

	void SetAspectRatio(float a) { asp = a; }

	mat4 GetViewMatrix()
	{
		vec3 w = (wEye - wLookat).normalize();
		vec3 u = cross(wVup, w).normalize();
		vec3 v = cross(w, u);

		return
			mat4(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				-wEye.x, -wEye.y, -wEye.z, 1.0f) *
			mat4(
				u.x, v.x, w.x, 0.0f,
				u.y, v.y, w.y, 0.0f,
				u.z, v.z, w.z, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);
	}

	mat4 GetProjectionMatrix()
	{
		float sy = 1 / tan(fov / 2);
		return mat4(
			sy / asp, 0.0f, 0.0f, 0.0f,
			0.0f, sy, 0.0f, 0.0f,
			0.0f, 0.0f, -(fp + bp) / (bp - fp), -1.0f,
			0.0f, 0.0f, -2 * fp*bp / (bp - fp), 0.0f);
	}
};

Camera* camera;

class Object
{
	Shader* shader;
	Mesh *mesh;

	vec3 position;
	vec3 scaling;
	float orientation;
	float angularV;
	float angularA;
	vec3 velocity = vec3(0, 0, 0);
	vec3 acceleration; 
	int ID;

	float rotation;

public:

	bool destroy = false;

	Object(Mesh *m, int inputID, vec3 position = vec3(0.0, 0.0, 0.0), vec3 scaling = vec3(1.0, 1.0, 1.0), float orientation = 0.0) : position(position), scaling(scaling), orientation(orientation)
	{
		shader = m->GetShader();
		mesh = m;
		ID = inputID;
	}

	vec3& GetPosition() { return position; }

	void DrawShadow(Shader* shadowShader) {
		shadowShader->Run();
		
		UploadAttributes(shadowShader);
		
		light->UploadAttributes(shadowShader);
		camera->UploadAttribtes(shadowShader);

		mesh->Draw();
	}

	void Draw()
	{
		shader->Run();
		camera->UploadAttribtes(shader);
		light->UploadAttributes(shader);
		UploadAttributes(shader);
		mesh->Draw();
	}

	void UploadAttributes(Shader* shader)
	{
		mat4 T = mat4(
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			position.x, position.y, position.z, 1.0);

		mat4 InvT = mat4(
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			-position.x, -position.y, -position.z, 1.0);

		mat4 S = mat4(
			scaling.x, 0.0, 0.0, 0.0,
			0.0, scaling.y, 0.0, 0.0,
			0.0, 0.0, scaling.z, 0.0,
			0.0, 0.0, 0.0, 1.0);

		mat4 InvS = mat4(
			1.0 / scaling.x, 0.0, 0.0, 0.0,
			0.0, 1.0 / scaling.y, 0.0, 0.0,
			0.0, 0.0, 1.0 / scaling.z, 0.0,
			0.0, 0.0, 0.0, 1.0);

		float alpha = orientation / 180.0 * M_PI;
		float beta = rotation / 180.0 * M_PI;

		mat4 R = mat4(
			cos(alpha), 0.0, sin(alpha), 0.0,
			0.0, 1.0, 0.0, 0.0,
			-sin(alpha), 0.0, cos(alpha), 0.0,
			0.0, 0.0, 0.0, 1.0);

		mat4 InvR = mat4(
			cos(alpha), 0.0, -sin(alpha), 0.0,
			0.0, 1.0, 0.0, 0.0,
			sin(alpha), 0.0, cos(alpha), 0.0,
			0.0, 0.0, 0.0, 1.0);

		vec3 u;
		if (!keyboardState['t']) {
			u = (camera->GetLookAt() - camera->GetwEye()).normalize(); //axis from which the obj rotates u.y=0
		}
		else {
			u = (camera->GetLookAt() - initialPos).normalize(); //axis from which the obj rotates u.y=0
		}


		mat4 Rz =
			mat4(cos(beta) + u.x*u.x*(1 - cos(beta)), -u.z*sin(beta), u.x*u.z*(1 - cos(beta)), 0,
				u.z*sin(beta), cos(beta), -u.x*sin(beta), 0,
				u.z*u.x*(1 - cos(beta)), u.x*sin(beta), cos(beta) + u.z*u.z*(1 - cos(beta)), 0,
				0, 0, 0, 1
			);

		mat4 InvRz =
			mat4(cos(beta) + u.x*u.x*(1 - cos(beta)), u.z*sin(beta), u.z*u.x*(1 - cos(beta)), 0,
				-u.z*sin(beta), cos(beta), u.x*sin(beta), 0,
				u.x*u.z*(1 - cos(beta)), -u.x*sin(beta), cos(beta) + u.z*u.z*(1 - cos(beta)), 0,
				0, 0, 0, 1
			);

		mat4 M = S * R * Rz * T;
		mat4 InvM = InvT * InvRz * InvR *  InvS;

		mat4 MVP = M * camera->GetViewMatrix() * camera->GetProjectionMatrix();
		mat4 VP = camera->GetViewMatrix() * camera->GetProjectionMatrix();

		shader->UploadVP(VP);
		shader->UploadInvM(InvM);
		shader->UploadMVP(MVP);
		shader->UploadM(M);
	}

	float getX()
	{
		return position.x;
	}

	float getY()
	{
		return position.y;
	}

	float getZ()
	{
		return position.z;
	}

	void setX(float x) {
		position.x = x;
	}

	void setY(float y) {
		position.y = y;
	}

	void setZ(float z) {
		position.z = z;
	}

	void movePositionX(float x)
	{
		position.x += x;
	}

	void movePositionY(float y)
	{
		position.y += y;
	}

	void movePositionZ(float z)
	{
		position.z += z;
	}

	int getID()
	{
		return ID;
	}

	void setVelocity(vec3 v) {
		velocity = v;
	}

	vec3 getVelocity() {
		return velocity;
	}

	bool CollisionCoin(Object *target) {
		if (target->getID() == 2) {
			vec3 pos = this->GetPosition();
			vec3 tarPos = target->GetPosition();
			return (pos - tarPos).length() < 0.5;
		}
		else {
			return false;
		}
		
	}

	bool CollisionTree(Object *target) {
		if (target->getID() == 3) {
			vec3 pos = this->GetPosition();
			vec3 tarPos = target->GetPosition();

			return (pos - tarPos).length() < 0.7;
		}
		else {
			return false;
		}
	}

	/* void MovePosition(vec3 pos, float dt) {
		
		vec3 dif = camera->GetLookAt() - camera->GetwEye();
		velocity = dif * (keyboardState['w'] - keyboardState['s']);
		position = position + velocity * dt;

		if (keyboardState['a'] || keyboardState['d']) {
			float angularSpeed = 180 / M_PI;
			angularV = keyboardState['a'] * -angularSpeed + keyboardState['d'] * angularSpeed;
			orientation += angularV * dt;

		}
		position = pos;
		float angularSpeed = 180 / M_PI;
		angularV = keyboardState['a'] * -angularSpeed + keyboardState['d'] * angularSpeed;
		orientation += angularV * dt;



		
	}*/

	void MovePosition(float dt) {

		//        vec3 dif = camera.GetLookat()-camera.GetwEye();

		//        velocity = dif * (keyboardState['w'] - keyboardState['s']);

		//        SetAcceleration();
		//        
		//        if(keyboardState['w'] or keyboardState['s']){
		//            velocity = velocity + acceleration * (keyboardState['w'] - keyboardState['s']);
		//        } else {
		//            velocity = velocity + acceleration;
		//        }
		//        

		//        position = position + velocity * dt;

		position = camera->GetLookAt();
		position.y = -0.8;

		if (keyboardState['a'] || keyboardState['d']) {
			float angularSpeed = 180 / M_PI;
			angularV = keyboardState['a'] * -angularSpeed + keyboardState['d'] * angularSpeed;
			orientation += angularV * dt;

		}
	}

	void frenet(float dt) {
		if (keyboardState['d']) {
			if (keyboardState['w']) {
				rotation -= 60 * dt;
				rotation = fmax(rotation, -5);

			}
			else if (keyboardState['s']) {
				rotation += 60 * dt;
				rotation = fmin(rotation, 5);

			}
			else {
				if (rotation>0) {
					rotation -= 60 * dt;
					rotation = fmax(rotation, 0);

				}
				else {
					rotation += 60 * dt;
					rotation = fmin(rotation, 0);

				}
			}
		}
		else if (keyboardState['a']) {
			if (keyboardState['w']) {
				rotation += 60 * dt;
				rotation = fmin(rotation, 5);

			}
			else if (keyboardState['s']) {
				rotation -= 60 * dt;
				rotation = fmax(rotation, -5);

			}
			else {
				if (rotation>0) {
					rotation -= 60 * dt;
					rotation = fmax(rotation, 0);

				}
				else {
					rotation += 60 * dt;
					rotation = fmin(rotation, 0);

				}
			}

		}
		else {
			if (rotation>0) {
				rotation -= 60 * dt;
				rotation = fmax(rotation, 0);
			}
			else {
				rotation += 60 * dt;
				rotation = fmin(rotation, 0);

			}
		}

	}

	void Rotate(float dt) {
		rotation += dt * 200;
	}

	void Spin(float dt)
	{
		orientation += 100 * dt;
	}

	void Move(float dt)
	{
		velocity = velocity + acceleration*dt;
		position = position + velocity *dt;
		angularV = angularV + angularA * dt;
		orientation = orientation + angularV * dt;
	}

	void SetOrientation(vec3 dir) {

		vec3 v1 = dir.normalize();
		vec3 v2 = vec3(0, 0, -1).normalize();

		float cos = v1.x*v2.x + v1.z*v2.z;


		float angle = acosf(cos) * 180 / M_PI;

		if (v1.x < 0) angle = -angle;


		orientation = angle + 180;
	}

	void Falling(float t, float dt) {
		velocity = vec3(cos(5 * t), -2, sin(5 * t)).normalize();
		SetOrientation(velocity);
		position = position + velocity * dt;
		
	}

};

/*class Avatar : public Object
{
	vec3 position;
	vec3 scaling;
	float orientation;
	float angularV;
	float angularA;
	vec3 velocity = vec3(0, 0, 0);
	vec3 acceleration;

public: Avatar(vec3 position, vec3 scaling, float orientation, vec3 velocity, vec3 acceleration) {
	
	
	void Control(float dt, vec3& avatar position)

	{
	velocity  = veloctiy * 0.999
	if (keyboardState['w']) {
	}
	}
	avatar class
	vec3 getahead()
	float alpha = (orientation plus 180 ) * pi / 180
	return vec3(cos(alpha), 0, sin(alpha))
}
};*/

Object* objectT;
Object* objectH;
Object* objectHA;
std::vector<Object*> objects;
int numCoin = 70;
int numTree = 200;

class Scene
{
	MeshShader* meshShader;
	InfiniteQuadShader* infiniteShader;
	ShadowShader* shadowShader;

	std::vector<Texture*> textures;
	std::vector<Material*> materials;
	std::vector<Geometry*> geometries;
	std::vector<Mesh*> meshes;
	//std::vector<Object*> objects;

public:
	Scene()
	{
		meshShader = 0;
	}

	double get_random(double min, double max) {
		/* Returns a random double between min and max */

		return (max - min) * ((double)rand() / (double)RAND_MAX);
	}

	void Initialize()
	{


		light = new Light(vec3(1, 1, 1), vec3(1, 1, 1), vec4(2.55, 2.55, 2.55, 0));
		spotlight = new Light(vec3(1, 1, 1), vec3(1, 1, 1), vec4(-0.1, -0.3, 0.1, 1.0));
		camera = new Camera();

		infiniteShader = new InfiniteQuadShader();
		shadowShader = new ShadowShader();
		meshShader = new MeshShader();

		textures.push_back(new Texture("tigger.png"));
		textures.push_back(new Texture("tree.png"));
		textures.push_back(new Texture("1-2-cowboy-hat-png-file-thumb.png"));
		textures.push_back(new Texture("chevy.png"));
		//textures.push_back(new Texture("grass.png"));
		textures.push_back(new Texture("ice_texture3006.jpg"));
		textures.push_back(new Texture("coin-texture.jpg"));
		//textures.push_back(new Texture("NewTexture.png"));


		materials.push_back(new Material(meshShader,
			vec3(.1, .1, .1), vec3(.9, .9, .9), vec3(0.0, 0.0, 0.0), 0,
			textures[0]));
		materials.push_back(new Material(meshShader,
			vec3(.1, .1, .1), vec3(.6, .6, .6), vec3(0.3, 0.3, 0.3), 50,
			textures[1]));
		materials.push_back(new Material(meshShader,
			vec3(.1, .1, .1), vec3(.9, .9, .9), vec3(0.0, 0.0, 0.0), 0,
			textures[2]));
		materials.push_back(new Material(meshShader,
			vec3(.1, .1, .1), vec3(.6, .6, .6), vec3(0.3, 0.3, 0.3), 50,
			textures[3]));
		materials.push_back(new Material(infiniteShader,
			vec3(.1, .1, .1), vec3(.9, .9, .9), vec3(0.0, 0.0, 0.0), 0,
			textures[4]));
		materials.push_back(new Material(meshShader,
			vec3(.1, .1, .1), vec3(.9, .9, .9), vec3(0.0, 0.0, 0.0), 0,
			textures[5]));
		/*materials.push_back(new Material(meshShader,
			vec3(.1, .1, .1), vec3(.9, .9, .9), vec3(0.0, 0.0, 0.0), 0,
			textures[3]));*/

		geometries.push_back(new PolygonalMesh("tigger.obj"));
		geometries.push_back(new PolygonalMesh("tree.obj"));
		geometries.push_back(new PolygonalMesh("chevy.obj"));
		geometries.push_back(new InfiniteTexturedQuad());
		geometries.push_back(new PolygonalMesh("tricoin.obj"));
		//geometries.push_back(new PolygonalMesh("bmwtriangles.obj"));
		

		meshes.push_back(new Mesh(geometries[0], materials[0]));
		meshes.push_back(new Mesh(geometries[1], materials[1]));
		meshes.push_back(new Mesh(geometries[1], materials[2]));
		meshes.push_back(new Mesh(geometries[2], materials[3]));
		meshes.push_back(new Mesh(geometries[3], materials[4]));
		meshes.push_back(new Mesh(geometries[4], materials[5]));
		//meshes.push_back(new Mesh(geometries[3], materials[3]));

		objectT = new Object(meshes[0], 1, vec3(0.0, -0.8, 0.0), vec3(0.015, 0.015, 0.015), 90.0);
		objects.push_back(objectT);

		for (int i = 0; i < numCoin; i++) {
			double random = get_random(-10.0, 10.0);
			double randoz = get_random(-10.0, 10.0);
			objects.push_back(new Object(meshes[5], 2, vec3(random, -0.5, randoz), vec3(.25, 0.25, 0.25), 90.0));
		}

		for (int i = 0; i < numTree; i++) {
			double random = get_random(-15.0, 15.0);
			double randoz = get_random(-15.0, 15.0);
			double random1 = get_random(0.0, 0.05);
			objects.push_back(new Object(meshes[1], 3, vec3(random, -1, randoz), vec3(random1, random1, random1), 0));
		}
		//objects.push_back(new Object(meshes[1], vec3(-.5, -1, -1), vec3(.03, .03, .03), 0));
		//objects.push_back(new Object(meshes[1], vec3(.5, -1, -.5), vec3(.02, .02, .02), 30));
		//objectH = new Object(meshes[2], vec3(0.0, 0.3, 0.0), vec3(.005, .005, .005), 90);
		//objects.push_back(objectH);
		objectHA = new Object(meshes[3], 4, vec3(0.0, -0.8, 0.0), vec3(.03, .03, .03), 180);
		objects.push_back(objectHA);
		objects.push_back(new Object(meshes[4], 5, vec3(0, -1, 0), vec3(1, 1, 1), 0));
		//objects.push_back(new Object(meshes[3], vec3(1, -.5, -.5), vec3(.02, .02, .02), 30));
		

	}

	~Scene()
	{
		for (int i = 0; i < textures.size(); i++) delete textures[i];
		for (int i = 0; i < materials.size(); i++) delete materials[i];
		for (int i = 0; i < geometries.size(); i++) delete geometries[i];
		for (int i = 0; i < meshes.size(); i++) delete meshes[i];
		for (int i = 0; i < objects.size(); i++) delete objects[i];

		if (meshShader) delete meshShader;
	}

	//void Update() {
	//	float m = .002;
	//	if (keyboardState['w']) {
	//		objectT->movePositionZ(-m);
	//	}
	//	if (keyboardState['s']) {
	//		objectT->movePositionZ(m);
	//	}
	//	if (keyboardState['a']) {
	//		objectT->movePositionX(-m);
	//	//}
	//	if (keyboardState['d']) {
	//		objectT->movePositionX(m);
	//	}
	//}

	void Draw()
	{
		for (int i = 0; i < objects.size(); i++) {
			//if (i != objects.size()-1)
				//objects[i]->DrawShadow(shadowShader);
			if (i != objects.size() - 1 && !objects[i]->destroy) {
				objects[i]->DrawShadow(shadowShader);
			}
			objects[i]->Draw();
		}
	}

};

Scene scene;

void onInitialization()
{
	glViewport(0, 0, windowWidth, windowHeight);

	scene.Initialize();
}

void onExit()
{
	printf("exit");
}

void onDisplay()
{

	glClearColor(0, 0, 1.0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	scene.Draw();

	glutSwapBuffers();

}

bool tPressed = false;
double tHeart = 0;

void onKeyboard(unsigned char key, int x, int y)
{
	keyboardState[key] = true;
	if (key == 't' && !tPressed) {
		tPressed = true;
		initialPos = camera->GetwEye();
		initialHat = objectHA->GetPosition();
	}
}

void onKeyboardUp(unsigned char key, int x, int y)
{
	keyboardState[key] = false;

	if (key == 't') {
		tPressed = false;
		camera->SetwEye(initialPos);
		tHeart = 0;
	}

}

void onReshape(int winWidth, int winHeight)
{
	camera->SetAspectRatio((float)winWidth / winHeight);
	glViewport(0, 0, winWidth, winHeight);
}

void onIdle() {
	double t = glutGet(GLUT_ELAPSED_TIME) * 0.001;
	static double lastTime = 0.0;
	double dt = t - lastTime;
	lastTime = t;

	if (keyboardState['t']) {
		tHeart += dt;
		camera->TrackingShot(tHeart);
	}

	//PlaySound(TEXT("Mario.mp3"), NULL, SND_FILENAME | SND_ASYNC);

	for (int i = 0; i < objects.size(); i++) {
		//1 : tigger
		//2 : coin 
		//3 : trees
		//4 : car
		if (objectT->CollisionCoin(objects[i]))
		{
			objects[i]->setY(-2);
			objects.erase(objects.begin()+ i);
			//objects[i]->Falling(t, dt);
			
			PlaySound(TEXT("coin.wav"), NULL, SND_FILENAME | SND_ASYNC);
			camera->setVelocity(camera->getVelocity()*1.67);
		}
		if (objectT->CollisionTree(objects[i]))
		{
			camera->setVelocity(camera->getVelocity() * -0.7);
			//objectHA->setVelocity(vec3(0,0,0));
			//how to bounce????
		}

	}
	

	//scene.Update();
	camera->Control();
	camera->Move(dt);
	//vec3 pos = vec3(camera->GetLookAt().x, -0.8, camera->GetLookAt().z);
	objectT->MovePosition(dt);
	objectT->Move(dt);
	objectT->Spin(dt*2);

	//vec3 pos1 = vec3(camera->GetLookAt().x, 0.3, camera->GetLookAt().z);
	//objectH->MovePosition(pos1, dt);
	//objectH->Spin(dt);

	//vec3 pos2 = vec3(camera->GetLookAt().x, -0.8, camera->GetLookAt().z);
	objectHA->MovePosition(dt);

	if (keyboardState['t']) {
		objectT->MovePosition(dt);
	}
	else {
		objectT->MovePosition(dt);
	}

	//vec3 dif = camera->GetLookAt() - camera->GetwEye();

	// fix the spotlight to tiggers position
	spotlight->SetPointLightSource(vec3(camera->GetLookAt().x, -1, camera->GetLookAt().z));
	//vec3 pos2 = vec3((dif.x*cos(-0.07) - dif.z*sin(-0.07))*0.9, -0.3, (dif.x*sin(-0.07) + dif.z*cos(-0.07)) * 0.9) + camera->GetwEye();
	//spotlight->SetPointLightSource(pos2);

	objectT->frenet(dt);
	//objectT->Rotate(0);

	/*objectH->frenet(dt);
	objectH->Rotate(0);
*/
	objectHA->frenet(dt);
	objectHA->Rotate(0);

	for (int i = 1; i < objects.size(); i++) {
		if (objects[i]->getID() == 2) {
			objects[i]->Rotate(dt);
		}
	}

//	for (int i = 0; i < objects.size(); i++) {
//// if tigger object collision
//		// then the switch 
//		if(objectT->Collision(objects[i])) {
//
//		switch (objects[i]->getID()) {
//			//1 : tigger
//			//2 : coin 
//			//3 : trees
//			//4 : car
//		case 2: 
//			objects[i]->Falling(t, dt);
//			break;
//		case 3: 
//			objectT->setX(objects[i]->getX());
//			objectHA->setX(objects[i]->getX());
//			objectT->setX(objects[i]->getX());
//			objectHA->setX(objects[i]->getX());
//		}
//
//	}


	glutPostRedisplay();
}

int main(int argc, char * argv[])
{
	glutInit(&argc, argv);
#if !defined(__APPLE__)
	glutInitContextVersion(majorVersion, minorVersion);
#endif
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(50, 50);
#if defined(__APPLE__)
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutCreateWindow("3D Mesh Rendering");

#if !defined(__APPLE__)
	glewExperimental = true;
	glewInit();
#endif
	printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
	printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
	printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
	printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	onInitialization();

	glutDisplayFunc(onDisplay);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutKeyboardUpFunc(onKeyboardUp);
	glutReshapeFunc(onReshape);

	glutMainLoop();
	onExit();
	return 1;
}

