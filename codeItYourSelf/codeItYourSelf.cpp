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
	float w = 1.0f;
};

struct Triangle {
	Vec3D p[3];
	wchar_t sym;
	short col;
};

struct Mesh {
	std::vector<Triangle> tris;

	bool LoadFromObjectFile(std::string sFilename)
	{
		std::ifstream f(sFilename);
		if (!f.is_open())
			return false;

		// Local cache of verts
		std::vector<Vec3D> verts;

		while (!f.eof())
		{
			char line[128];
			f.getline(line, 128);

			std::strstream s;
			s << line;

			char junk;

			if (line[0] == 'v')
			{
				Vec3D v;
				s >> junk >> v.x >> v.y >> v.z;
				verts.push_back(v);
			}

			if (line[0] == 'f')
			{
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
	float fTheta = 30.f;
	Mesh meshCube;
	Mat4x4 matProj;
	Vec3D camera;
	Vec3D vLookDir;
	float fYaw;
	Vec3D Matrix_MultiplyVector(Mat4x4& m, Vec3D& i)
	{
		Vec3D v;
		v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
		v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
		v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
		v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];
		return v;
	}

	Mat4x4 Matrix_MakeIdentity()
	{
		Mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	Mat4x4 Matrix_MakeRotationX(float fAngleRad)
	{
		Mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = cosf(fAngleRad);
		matrix.m[1][2] = sinf(fAngleRad);
		matrix.m[2][1] = -sinf(fAngleRad);
		matrix.m[2][2] = cosf(fAngleRad);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	Mat4x4 Matrix_MakeRotationY(float fAngleRad)
	{
		Mat4x4 matrix;
		matrix.m[0][0] = cosf(fAngleRad);
		matrix.m[0][2] = sinf(fAngleRad);
		matrix.m[2][0] = -sinf(fAngleRad);
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = cosf(fAngleRad);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	Mat4x4 Matrix_MakeRotationZ(float fAngleRad)
	{
		Mat4x4 matrix;
		matrix.m[0][0] = cosf(fAngleRad);
		matrix.m[0][1] = sinf(fAngleRad);
		matrix.m[1][0] = -sinf(fAngleRad);
		matrix.m[1][1] = cosf(fAngleRad);
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	Mat4x4 Matrix_MakeTranslation(float x, float y, float z)
	{
		Mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		matrix.m[3][0] = x;
		matrix.m[3][1] = y;
		matrix.m[3][2] = z;
		return matrix;
	}

	Mat4x4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
	{
		float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
		Mat4x4 matrix;
		matrix.m[0][0] = fAspectRatio * fFovRad;
		matrix.m[1][1] = fFovRad;
		matrix.m[2][2] = fFar / (fFar - fNear);
		matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matrix.m[2][3] = 1.0f;
		matrix.m[3][3] = 0.0f;
		return matrix;
	}

	Mat4x4 Matrix_MultiplyMatrix(Mat4x4& m1, Mat4x4& m2)
	{
		Mat4x4 matrix;
		for (int c = 0; c < 4; c++)
			for (int r = 0; r < 4; r++)
				matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
		return matrix;
	}

	Mat4x4 Matrix_PointAt(Vec3D& pos, Vec3D& target, Vec3D& up)
	{
		// Calculate new forward direction
		Vec3D newForward = Vector_Sub(target, pos);
		newForward = Vector_Normalise(newForward);

		// Calculate new Up direction
		Vec3D a = Vector_Mul(newForward, Vector_DotProduct(up, newForward));
		Vec3D newUp = Vector_Sub(up, a);
		newUp = Vector_Normalise(newUp);

		// New Right direction is easy, its just cross product
		Vec3D newRight = Vector_CrossProduct(newUp, newForward);

		// Construct Dimensioning and Translation Matrix	
		Mat4x4 matrix;
		matrix.m[0][0] = newRight.x;	matrix.m[0][1] = newRight.y;	matrix.m[0][2] = newRight.z;	matrix.m[0][3] = 0.0f;
		matrix.m[1][0] = newUp.x;		matrix.m[1][1] = newUp.y;		matrix.m[1][2] = newUp.z;		matrix.m[1][3] = 0.0f;
		matrix.m[2][0] = newForward.x;	matrix.m[2][1] = newForward.y;	matrix.m[2][2] = newForward.z;	matrix.m[2][3] = 0.0f;
		matrix.m[3][0] = pos.x;			matrix.m[3][1] = pos.y;			matrix.m[3][2] = pos.z;			matrix.m[3][3] = 1.0f;
		return matrix;

	}

	Mat4x4 Matrix_QuickInverse(Mat4x4& m) // Only for Rotation/Translation Matrices
	{
		Mat4x4 matrix;
		matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
		matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
		matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
		matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
		matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
		matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	Vec3D Vector_Add(Vec3D& v1, Vec3D& v2)
	{
		return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
	}

	Vec3D Vector_Sub(Vec3D& v1, Vec3D& v2)
	{
		return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	}

	Vec3D Vector_Mul(Vec3D& v1, float k)
	{
		return { v1.x * k, v1.y * k, v1.z * k };
	}

	Vec3D Vector_Div(Vec3D& v1, float k)
	{
		return { v1.x / k, v1.y / k, v1.z / k };
	}

	float Vector_DotProduct(Vec3D& v1, Vec3D& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	float Vector_Length(Vec3D& v)
	{
		return sqrtf(Vector_DotProduct(v, v));
	}

	Vec3D Vector_Normalise(Vec3D& v)
	{
		float l = Vector_Length(v);
		return { v.x / l, v.y / l, v.z / l };
	}

	Vec3D Vector_CrossProduct(Vec3D& v1, Vec3D& v2)
	{
		Vec3D v;
		v.x = v1.y * v2.z - v1.z * v2.y;
		v.y = v1.z * v2.x - v1.x * v2.z;
		v.z = v1.x * v2.y - v1.y * v2.x;
		return v;
	}

	Vec3D Vector_IntersectPlane(Vec3D& plane_p, Vec3D& plane_n, Vec3D& lineStart, Vec3D& lineEnd)
	{
		plane_n = Vector_Normalise(plane_n);
		float plane_d = -Vector_DotProduct(plane_n, plane_p);
		float ad = Vector_DotProduct(lineStart, plane_n);
		float bd = Vector_DotProduct(lineEnd, plane_n);
		float t = (-plane_d - ad) / (bd - ad);
		Vec3D lineStartToEnd = Vector_Sub(lineEnd, lineStart);
		Vec3D lineToIntersect = Vector_Mul(lineStartToEnd, t);
		return Vector_Add(lineStart, lineToIntersect);
	}

	int Triangle_ClipAgainstPlane(Vec3D plane_p, Vec3D plane_n, Triangle& in_tri, Triangle& out_tri1, Triangle& out_tri2)
	{
		// Make sure plane normal is indeed normal
		plane_n = Vector_Normalise(plane_n);

		// Return signed shortest distance from point to plane, plane normal must be normalised
		auto dist = [&](Vec3D& p)
		{
			Vec3D n = Vector_Normalise(p);
			return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - Vector_DotProduct(plane_n, plane_p));
		};

		// Create two temporary storage arrays to classify points either side of plane
		// If distance sign is positive, point lies on "inside" of plane
		Vec3D* inside_points[3];  int nInsidePointCount = 0;
		Vec3D* outside_points[3]; int nOutsidePointCount = 0;

		// Get signed distance of each point in Triangle to plane
		float d0 = dist(in_tri.p[0]);
		float d1 = dist(in_tri.p[1]);
		float d2 = dist(in_tri.p[2]);

		if (d0 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[0]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[0]; }
		if (d1 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[1]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[1]; }
		if (d2 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[2]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[2]; }

		// Now classify Triangle points, and break the input Triangle into 
		// smaller output Triangles if required. There are four possible
		// outcomes...

		if (nInsidePointCount == 0)
		{
			// All points lie on the outside of plane, so clip whole Triangle
			// It ceases to exist

			return 0; // No returned Triangles are valid
		}

		if (nInsidePointCount == 3)
		{
			// All points lie on the inside of plane, so do nothing
			// and allow the Triangle to simply pass through
			out_tri1 = in_tri;

			return 1; // Just the one returned original Triangle is valid
		}

		if (nInsidePointCount == 1 && nOutsidePointCount == 2)
		{
			// Triangle should be clipped. As two points lie outside
			// the plane, the Triangle simply becomes a smaller Triangle

			// Copy appearance info to new Triangle
			out_tri1.col = FG_GREEN;//in_tri.col;
			out_tri1.sym = in_tri.sym;

			// The inside point is valid, so keep that...
			out_tri1.p[0] = *inside_points[0];

			// but the two new points are at the locations where the 
			// original sides of the Triangle (lines) intersect with the plane
			out_tri1.p[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
			out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

			return 1; // Return the newly formed single Triangle
		}

		if (nInsidePointCount == 2 && nOutsidePointCount == 1)
		{
			// Triangle should be clipped. As two points lie inside the plane,
			// the clipped Triangle becomes a "quad". Fortunately, we can
			// represent a quad with two new Triangles

			// Copy appearance info to new Triangles
			out_tri1.col = FG_RED;//in_tri.col;
			out_tri1.sym = in_tri.sym;

			out_tri2.col = FG_BLUE;//in_tri.col;
			out_tri2.sym = in_tri.sym;

			// The first Triangle consists of the two inside points and a new
			// point determined by the location where one side of the Triangle
			// intersects with the plane
			out_tri1.p[0] = *inside_points[0];
			out_tri1.p[1] = *inside_points[1];
			out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

			// The second Triangle is composed of one of he inside points, a
			// new point determined by the intersection of the other side of the 
			// Triangle and the plane, and the newly created point above
			out_tri2.p[0] = *inside_points[1];
			out_tri2.p[1] = out_tri1.p[2];
			out_tri2.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

			return 2; // Return two newly formed Triangles which form a quad
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
		meshCube.LoadFromObjectFile("axis.txt");

		//Projection Matrix
		matProj = Matrix_MakeProjection(90.0f, (float)ScreenHeight() / (float)ScreenWidth(), 0.1f, 1000.0f);
		return true;
	}
	bool OnUserUpdate(float fElapsedTime) override {

		if (GetKey(VK_UP).bHeld) {
			camera.y += 8.f * fElapsedTime;
		}
		if (GetKey(VK_DOWN).bHeld) {
			camera.y -= 8.f * fElapsedTime;

		}
		if (GetKey(VK_LEFT).bHeld) {
			camera.x -= 8.f * fElapsedTime;
		}
		if (GetKey(VK_RIGHT).bHeld) {
			camera.x += 8.f * fElapsedTime;

		}

		Vec3D vForward = Vector_Mul(vLookDir, 8.f * fElapsedTime);

		if (GetKey(L'A').bHeld) {
			fYaw -= 2.f * fElapsedTime;
		}
		if (GetKey(L'D').bHeld) {
			fYaw += 2.f * fElapsedTime;

		}
		if (GetKey(L'W').bHeld) {
			camera = Vector_Add(camera, vForward);
		}
		if (GetKey(L'S').bHeld) {
			camera = Vector_Sub(camera, vForward);

		}

		Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

		Mat4x4 matRotZ, matRotX;
		//fTheta += 1.0f * fElapsedTime;

		matRotZ = Matrix_MakeRotationZ(fTheta * 0.5f);
		matRotX = Matrix_MakeRotationX(fTheta);

		Mat4x4 matTrans;
		matTrans = Matrix_MakeTranslation(0.f, 0.f, 16.f);

		Mat4x4 matWorld;
		matWorld = Matrix_MakeIdentity();
		matWorld = Matrix_MultiplyMatrix(matRotZ, matRotX);
		matWorld = Matrix_MultiplyMatrix(matWorld, matTrans);

		Vec3D vUp = { 0,1,0 };
		
		Vec3D vTarget = { 0,0,1 };
		Mat4x4 matCameraRot = Matrix_MakeRotationY(fYaw);
		vLookDir = Matrix_MultiplyVector(matCameraRot, vTarget);
		vTarget = Vector_Add(camera, vLookDir);

		Mat4x4 matCamera = Matrix_PointAt(camera, vTarget, vUp);
		Mat4x4 matView = Matrix_QuickInverse(matCamera);

		std::vector<Triangle> vecTrianglesToRaster;

		// Draw Triangles
		for (auto& tri : meshCube.tris) {
			Triangle triProjected, triTransformed, triView;

			triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
			triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
			triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

			Vec3D normal, line1, line2;
			line1 = Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
			line2 = Vector_Sub(triTransformed.p[2], triTransformed.p[0]);

			normal = Vector_CrossProduct(line1, line2);

			normal = Vector_Normalise(normal);

			Vec3D vCameraRay = Vector_Sub(triTransformed.p[0], camera);


			//if (normal.z < 0) 
			if(Vector_DotProduct(normal, vCameraRay)< 0.f)
			{
				
				Vec3D light_direction = { 0.0f, 0.0f, -1.0f };
				light_direction = Vector_Normalise(light_direction);

				// How "aligned" are light direction and Triangle normal?
				float dp = max(0.1f, Vector_DotProduct(light_direction, normal));

				CHAR_INFO c = GetColour(dp);
				triTransformed.col = c.Attributes;
				triTransformed.sym = c.Char.UnicodeChar;

				triView.p[0] = Matrix_MultiplyVector(matView, triTransformed.p[0]);
				triView.p[1] = Matrix_MultiplyVector(matView, triTransformed.p[1]);
				triView.p[2] = Matrix_MultiplyVector(matView, triTransformed.p[2]);

				int nClippedTriangles = 0;
				Triangle clipped[2];
				nClippedTriangles = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 2.1f }, { 0.0f, 0.0f, 1.0f }, triView, clipped[0], clipped[1]);

				for (int n = 0; n < nClippedTriangles; n++) {



					triProjected.p[0] = Matrix_MultiplyVector(matProj, clipped[n].p[0]);
					triProjected.p[1] = Matrix_MultiplyVector(matProj, clipped[n].p[1]);
					triProjected.p[2] = Matrix_MultiplyVector(matProj, clipped[n].p[2]);
					triProjected.col = triTransformed.col;
					triProjected.sym = triTransformed.sym;

					// Perform Homogeneous divide
					triProjected.p[0] = Vector_Div(triProjected.p[0], triProjected.p[0].w);
					triProjected.p[1] = Vector_Div(triProjected.p[1], triProjected.p[1].w);
					triProjected.p[2] = Vector_Div(triProjected.p[2], triProjected.p[2].w);

					// X/Y are inverted so put them back
					triProjected.p[0].x *= -1.0f;
					triProjected.p[1].x *= -1.0f;
					triProjected.p[2].x *= -1.0f;
					triProjected.p[0].y *= -1.0f;
					triProjected.p[1].y *= -1.0f;
					triProjected.p[2].y *= -1.0f;

					Vec3D offsetView = { 1,1,0 };
					triProjected.p[0] = Vector_Add(triProjected.p[0], offsetView);
					triProjected.p[1] = Vector_Add(triProjected.p[1], offsetView);
					triProjected.p[2] = Vector_Add(triProjected.p[2], offsetView);

					triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
					triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
					triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
					triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
					triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
					triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

					// Store Triangles for sorting
					vecTrianglesToRaster.push_back(triProjected);
				}
				
			}
		}

		std::sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](Triangle& t1, Triangle& t2) {
			float z1 = (t1.p[0].z + t1.p[1].x + t1.p[2].z) / 3.f;
			float z2 = (t2.p[0].z + t2.p[1].x + t2.p[2].z) / 3.f;
			return z1 > z2;
			});

		for (auto& triToRaster : vecTrianglesToRaster) {
			Triangle clipped[2];
			std::list<Triangle> listTriangles;
			listTriangles.push_back(triToRaster);
			int nNewTriangles = 1;

			for (int p = 0; p < 4; p++)
			{
				int nTrisToAdd = 0;
				while (nNewTriangles > 0)
				{
					// Take triangle from front of queue
					Triangle test = listTriangles.front();
					listTriangles.pop_front();
					nNewTriangles--;

					// Clip it against a plane. We only need to test each 
					// subsequent plane, against subsequent new triangles
					// as all triangles after a plane clip are guaranteed
					// to lie on the inside of the plane. I like how this
					// comment is almost completely and utterly justified
					switch (p)
					{
					case 0:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					case 1:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, (float)ScreenHeight() - 1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					case 2:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					case 3:	nTrisToAdd = Triangle_ClipAgainstPlane({ (float)ScreenWidth() - 1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					}

					// Clipping may yield a variable number of triangles, so
					// add these new ones to the back of the queue for subsequent
					// clipping against next planes
					for (int w = 0; w < nTrisToAdd; w++)
						listTriangles.push_back(clipped[w]);
				}
				nNewTriangles = listTriangles.size();
			}
			/*FillTriangle(triProjected.p[0].x, triProjected.p[0].y,
				triProjected.p[1].x, triProjected.p[1].y,
				triProjected.p[2].x, triProjected.p[2].y,
				triProjected.sym, triProjected.col);

			DrawTriangle(triProjected.p[0].x, triProjected.p[0].y,
				triProjected.p[1].x, triProjected.p[1].y,
				triProjected.p[2].x, triProjected.p[2].y,
				PIXEL_SOLID, FG_WHITE);*/
				// Draw the transformed, viewed, clipped, projected, sorted, clipped triangles
			for (auto& t : listTriangles)
			{
				FillTriangle(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, t.p[2].x, t.p[2].y, t.sym, t.col);
				DrawTriangle(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, t.p[2].x, t.p[2].y, PIXEL_SOLID, FG_BLACK);
			}
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