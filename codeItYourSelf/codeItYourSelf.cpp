#include <iostream>
#include <string>
#include "olcConsoleGameEngine.h"
#include <vector>
#include <fstream>
#include <strstream>
#include <algorithm>

struct Vec3D {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

struct Triangle {
	Vec3D p[3];
	wchar_t sym;
	short col;
};

struct Mesh {
	std::vector<Triangle> tris;

	bool loadFromObjectFile(std::string sFilename) {
		std::ifstream f(sFilename);
		if (!f.is_open()) {
			return false;
		}
		//local cache of verts
		std::vector<Vec3D> verts;
		while (!f.eof()) {
			char line[128];
			f.getline(line, 128);

			std::strstream s;
			s << line;

			char junk;
			if (line[0] == 'v') {
				Vec3D v;
				s >> junk >> v.x >> v.y >> v.z;
				verts.push_back(v);
			}

			if (line[0] == 'f') {
				int f[3];
				s >> junk >> f[0] >> f[1] >> f[2];
				tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });

			}
		}
		return true;
	}
};

struct Mat4x4 {
	float m[4][4] = { 0 };
};


class olcEngine3D : public olcConsoleGameEngine {
public:
	olcEngine3D() {
		m_sAppName = L"3D Engine Demo";
	}

private:
	float fTheta = 0.f;
	Mesh meshCube;
	Mat4x4 matProj;
	Vec3D camera;
	 
	void MultiplyMatrixVector(Vec3D& i, Vec3D& o, Mat4x4& matrix) {
		o.x = i.x * matrix.m[0][0] + i.y * matrix.m[1][0] + i.z * matrix.m[2][0] + matrix.m[3][0];
		o.y = i.x * matrix.m[0][1] + i.y * matrix.m[1][1] + i.z * matrix.m[2][1] + matrix.m[3][1];
		o.z = i.x * matrix.m[0][2] + i.y * matrix.m[1][2] + i.z * matrix.m[2][2] + matrix.m[3][2];
		float w = i.x * matrix.m[0][3] + i.y * matrix.m[1][3] + i.z * matrix.m[2][3] + matrix.m[3][3];

		if (w != 0.f) {
			o.x /= w; o.y /= w; o.z /= w;
		}
	}

	// Taken From Command Line Webcam Video
	CHAR_INFO GetColour(float lum)
	{
		short bg_col, fg_col;
		wchar_t sym;
		int pixel_bw = (int)(13.0f * lum);
		switch (pixel_bw)
		{
		case 0: bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID; break;

		case 1: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_QUARTER; break;
		case 2: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_HALF; break;
		case 3: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_THREEQUARTERS; break;
		case 4: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_SOLID; break;

		case 5: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_QUARTER; break;
		case 6: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_HALF; break;
		case 7: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_THREEQUARTERS; break;
		case 8: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_SOLID; break;

		case 9:  bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_QUARTER; break;
		case 10: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_HALF; break;
		case 11: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_THREEQUARTERS; break;
		case 12: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_SOLID; break;
		default:
			bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID;
		}

		CHAR_INFO c;
		c.Attributes = bg_col | fg_col;
		c.Char.UnicodeChar = sym;
		return c;
	}
public:
	bool OnUserCreate() override {
		meshCube.loadFromObjectFile("VideoShip.txt");
		//meshCube.tris = {
		//	//South
		//	{ 0.0f, 0.0f, 0.0f,		0.0f, 1.0f, 0.0f,	1.0f, 1.0f, 0.0f },
		//	{ 0.0f, 0.0f, 0.0f,		1.0f, 1.0f, 0.0f,	1.0f, 0.0f, 0.0f },

		//	// East
		//	{ 1.0f, 0.0f, 0.0f,		1.0f, 1.0f, 0.0f,	1.0f, 1.0f, 1.0f },
		//	{ 1.0f, 0.0f, 0.0f,		1.0f, 1.0f, 1.0f,	1.0f, 0.0f, 1.0f },

		//	// North
		//	{ 1.0f, 0.0f, 1.0f,		1.0f, 1.0f, 1.0f,	0.0f, 1.0f, 1.0f },
		//	{ 1.0f, 0.0f, 1.0f,		0.0f, 1.0f, 1.0f,	0.0f, 0.0f, 1.0f },

		//	// West
		//	{ 0.0f, 0.0f, 1.0f,		0.0f, 1.0f, 1.0f,	0.0f, 1.0f, 0.0f },
		//	{ 0.0f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f,	0.0f, 0.0f, 0.0f },

		//	// Top
		//	{ 0.0f, 1.0f, 0.0f,		0.0f, 1.0f, 1.0f,	1.0f, 1.0f, 1.0f },
		//	{ 0.0f, 1.0f, 0.0f,		1.0f, 1.0f, 1.0f,	1.0f, 1.0f, 0.0f },

		//	// Bottom
		//	{ 1.0f, 0.0f, 1.0f,		0.0f, 0.0f, 1.0f,	0.0f, 0.0f, 0.0f },
		//	{ 1.0f, 0.0f, 1.0f,		0.0f, 0.0f, 0.0f,	1.0f, 0.0f, 0.0f },

		//};

		//Projection Matrix
		float fNear = 0.1f;
		float fFar = 1000.0f;
		float fFov = 90.0f;
		float fAspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
		float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);
	
		matProj.m[0][0] = fAspectRatio * fFovRad;
		matProj.m[1][1] = fFovRad;
		matProj.m[2][2] = fFar / (fFar - fNear);
		matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matProj.m[2][3] = 1.0f;
		matProj.m[3][3] = 0.f;
		return true;
	}
	bool OnUserUpdate(float fElapsedTime) override {
		Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

		Mat4x4 matRotz, matRotX;
		fTheta += 1.0f * fElapsedTime;

		matRotz.m[0][0] = cosf(fTheta);
		matRotz.m[0][1] = sinf(fTheta);
		matRotz.m[1][0] = -sinf(fTheta);
		matRotz.m[1][1] = cosf(fTheta);
		matRotz.m[2][2] = 1;
		matRotz.m[3][3] = 1;

		// Rotation X
		matRotX.m[0][0] = 1;
		matRotX.m[1][1] = cosf(fTheta * 0.5f); // Fixed index
		matRotX.m[1][2] = sinf(fTheta * 0.5f); // Fixed index
		matRotX.m[2][1] = -sinf(fTheta * 0.5f); // Fixed index
		matRotX.m[2][2] = cosf(fTheta * 0.5f); // Fixed index
		matRotX.m[3][3] = 1;

		std::vector<Triangle> vecTrianglesToRaster;

		// Draw Triangles
		for (auto& tri : meshCube.tris) {
			Triangle triProjected, triTranslated, triRotatedZ, triRotatedZX;

			MultiplyMatrixVector(tri.p[0], triRotatedZ.p[0], matRotz);
			MultiplyMatrixVector(tri.p[1], triRotatedZ.p[1], matRotz);
			MultiplyMatrixVector(tri.p[2], triRotatedZ.p[2], matRotz);

			MultiplyMatrixVector(triRotatedZ.p[0], triRotatedZX.p[0], matRotX);
			MultiplyMatrixVector(triRotatedZ.p[1], triRotatedZX.p[1], matRotX);
			MultiplyMatrixVector(triRotatedZ.p[2], triRotatedZX.p[2], matRotX);

			triTranslated = triRotatedZX;
			triTranslated.p[0].z = triRotatedZX.p[0].z + 8.f;
			triTranslated.p[1].z = triRotatedZX.p[1].z + 8.f;
			triTranslated.p[2].z = triRotatedZX.p[2].z + 8.f;

			Vec3D normal, line1, line2;
			line1.x = triTranslated.p[1].x - triTranslated.p[0].x;
			line1.y = triTranslated.p[1].y - triTranslated.p[0].y;
			line1.z = triTranslated.p[1].z - triTranslated.p[0].z;

			line2.x = triTranslated.p[2].x - triTranslated.p[0].x;
			line2.y = triTranslated.p[2].y - triTranslated.p[0].y;
			line2.z = triTranslated.p[2].z - triTranslated.p[0].z;

			normal.x = line1.y * line2.z - line1.z * line2.y;
			normal.y = line1.z * line2.x - line1.x * line2.z;
			normal.z = line1.x * line2.y - line1.y * line2.x;

			float l = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
			normal.x /= l; normal.y /= l; normal.z /= l;

			//if (normal.z < 0) 
			if(	normal.x * (triTranslated.p[0].x - camera.x) + 
				normal.y * (triTranslated.p[0].y - camera.y) +
				normal.z * (triTranslated.p[0].z - camera.z) < 0.0f)
			{
				//illumination
				Vec3D light_direction = { 0.f, 0.f, -1.f };
				float l = sqrtf(light_direction.x * light_direction.x + light_direction.y * light_direction.y + light_direction.z * light_direction.z);
				light_direction.x /= l; light_direction.y /= l; light_direction.z /= l;

				float dp = normal.x * light_direction.x + normal.y * light_direction.y + normal.z * light_direction.z;

				CHAR_INFO c = GetColour(dp);
				triTranslated.col = c.Attributes;
				triTranslated.sym = c.Char.UnicodeChar;


				MultiplyMatrixVector(triTranslated.p[0], triProjected.p[0], matProj);
				MultiplyMatrixVector(triTranslated.p[1], triProjected.p[1], matProj);
				MultiplyMatrixVector(triTranslated.p[2], triProjected.p[2], matProj);
				triProjected.col = triTranslated.col ;
				triProjected.sym = triTranslated.sym;

				triProjected.p[0].x += 1.f; triProjected.p[0].y += 1.f;
				triProjected.p[1].x += 1.f; triProjected.p[1].y += 1.f;
				triProjected.p[2].x += 1.f; triProjected.p[2].y += 1.f;

				triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
				triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
				triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

				// Store triangles for sorting
				vecTrianglesToRaster.push_back(triProjected);

				
			}
		}

		std::sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](Triangle& t1, Triangle& t2) {
			float z1 = (t1.p[0].z + t1.p[1].x + t1.p[2].z) / 3.f;
			float z2 = (t2.p[0].z + t2.p[1].x + t2.p[2].z) / 3.f;
			return z1 > z2;
			});

		for (auto& triProjected : vecTrianglesToRaster) {
			FillTriangle(triProjected.p[0].x, triProjected.p[0].y,
				triProjected.p[1].x, triProjected.p[1].y,
				triProjected.p[2].x, triProjected.p[2].y,
				triProjected.sym, triProjected.col);

			/*DrawTriangle(triProjected.p[0].x, triProjected.p[0].y,
				triProjected.p[1].x, triProjected.p[1].y,
				triProjected.p[2].x, triProjected.p[2].y,
				PIXEL_SOLID, FG_WHITE);*/
		}
		return true;
	}
};

int main()
{
	olcEngine3D demo;
	if (demo.ConstructConsole(256, 240, 4, 2))
	{
		demo.Start();
	}
}