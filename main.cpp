#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <Windows.h>
#include <math.h>
#include <chrono>
#include <cmath>
#include <vector>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int WINDOW_HEIGHT = 800;
int WINDOW_WIDTH  = 800;

float rotTheta= 0.0f;

struct Matrix4x4
{
	float matrix[4][4] = { 0 };

	void InitalizeMatrix(const float (&values)[4][4])
	{
		for (int i = 0;i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				matrix[i][j] = values[i][j];
			}
		}
	}
};

typedef struct {
	float x;
	float y;
	float z;
	float h;
}vertex;

struct vector3D
{
	// size & direction
	float x;
	float y;
	float z;

	float CalculateSize()
	{
		return std:: sqrtf(x*x + y*y + z*z);
	}

};

typedef struct
{
	vertex v[3];
}triangle;

//struct model
//{
//	std:: vector<triangle> triangles;
//
//	bool LoadObjFile(const std::string& filePath)
//	{
//		std::ifstream f(filePath);
//		if (!f.is_open()) {
//			std::cerr << "Failed to open OBJ file: " << filePath << std::endl;
//			return false;
//		}
//
//		std::vector<vertex> verts;
//
//		std:: string line;
//		while (std::getline(f, line))
//		{
//			std:: stringstream s(line);
//			char junk;
//
//			// Check for vertex lines ('v ')
//			if (line.length() >= 2 && line[0] == 'v' && line[1] == ' ')
//			{
//				vertex v;
//				s >> junk >> v.x >> v.y >> v.z;
//				verts.push_back(v);
//			}
//
//			// Check for face lines ('f ')
//			if (line.length() >= 2 && line[0] == 'f' && line[1] == ' ')
//			{
//				int f_idx[3];
//				s >> junk >> f_idx[0] >> f_idx[1] >> f_idx[2];
//
//				// Subtract 1 because OBJ files start counting at 1, but C++ starts at 0
//				triangles.push_back({ verts[f_idx[0] - 1], verts[f_idx[1] - 1], verts[f_idx[2] - 1] });
//			}
//		}
//
//		f.close();
//		return true;
//
//	}
//
//};

struct model
{
	std::vector<triangle> triangles;

	bool LoadObjFile(const std::string& filePath)
	{
		std::ifstream f(filePath);
		if (!f.is_open()) {
			std::cerr << "Failed to open OBJ file: " << filePath << std::endl;
			return false;
		}

		std::vector<vertex> verts;
		std::string line;

		while (std::getline(f, line))
		{
			std::stringstream s(line);
			char junk;

			// Check for vertex lines ('v ')
			if (line.length() >= 2 && line[0] == 'v' && line[1] == ' ')
			{
				vertex v;
				s >> junk >> v.x >> v.y >> v.z;
				v.h = 1.0f; // Important since your vertex struct has 4 floats (x, y, z, h)
				verts.push_back(v);
			}

			// Check for face lines ('f ')
			if (line.length() >= 2 && line[0] == 'f' && line[1] == ' ')
			{
				s >> junk; // Throw away 'f'
				std::string token;
				int f_idx[3];
				bool validFace = true;

				for (int i = 0; i < 3; ++i)
				{
					if (!(s >> token)) {
						validFace = false;
						break;
					}

					// If token has a slash (e.g. "308/1/1"), extract just the number before the slash
					size_t slashPos = token.find('/');
					std::string indexStr = (slashPos != std::string::npos) ? token.substr(0, slashPos) : token;

					// Convert to 0-based index
					int idx = std::stoi(indexStr) - 1;

					// Safety check: Make sure this vertex actually exists before crashing
					if (idx >= 0 && idx < verts.size()) {
						f_idx[i] = idx;
					}
					else {
						validFace = false;
						break;
					}
				}

				// Only add the triangle if all 3 indices were successfully read and validated
				if (validFace) {
					triangles.push_back({ verts[f_idx[0]], verts[f_idx[1]], verts[f_idx[2]] });
				}
			}
		}

		f.close();
		return true;
	}
};

enum ColorsWin32 : uint32_t
{
	Red		= 0x00FF0000, 
	Green	= 0x0000FF00,
	Blue	= 0x000000FF,
	Yellow	= 0x00FFFF00,
	Purple	= 0x00800080,
	Cyan	= 0x0000FFFF,
	Orange	= 0x00FF8000,
	White	= 0x00FFFFFF,
	Pink	= 0x00FF1493,
	Gray	= 0x00808080,
	Lime	= 0x0080FF00,
	Black	= 0x00000000
};


const wchar_t windowClassName[] = L"CubeWindow";
uint32_t* pixel_buffer;
BITMAPINFO bmpi;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		StretchDIBits(
			hdc,
			0,
			0,
			WINDOW_WIDTH,
			WINDOW_HEIGHT,
			0,
			0,
			WINDOW_WIDTH,
			WINDOW_HEIGHT,
			pixel_buffer,
			&bmpi,
			DIB_RGB_COLORS,
			SRCCOPY
		);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void swap_points2d(vertex* point1, vertex*  point2)
{
	vertex temp = *point1;
	*point1 = *point2;
	*point2 = temp;
}

uint32_t ShadeColor(uint32_t color, float intensity)
{
	// extract color channels
	//   00000001 10101001 10011010 10101010  (32 - bit color number)
	// & 00000000 00000000 00000000 11111111  (0xFF mask)
	uint32_t r = (color >> 16) & 0xFF;
	uint32_t g = (color >> 8) & 0xFF;
	uint32_t b = color & 0xFF;

	if (intensity < 0.0f) intensity = 0.0f;
	if (intensity > 1.0f) intensity = 1.0f;

	//intensity = std::clamp(intensity, 0.0f, 1.0f);

	uint32_t shaded_r = (uint32_t)(r * intensity);
	uint32_t shaded_g = (uint32_t)(g * intensity);
	uint32_t shaded_b = (uint32_t)(b * intensity);

	return (shaded_r << 16) | (shaded_g << 8) | shaded_b;
}

void DrawRectangle(vertex sCoord, int w, int h, uint32_t color)
{
	for (int py = sCoord.y; py < sCoord.y + h; py++)
	{
		for (int px = sCoord.x; px < sCoord.x + w; px++)
		{
			int i = py * WINDOW_WIDTH + px;
			pixel_buffer[i] = color;
		}
	}
}

void DrawCircle(vertex sCoord, int radius, uint32_t color)
{
	int x_distance = 0;
	int y_distance = 0;
	double point_distance=0.0;
	for (int py = 0; py < WINDOW_HEIGHT; py++)
	{
		for (int px = 0; px < WINDOW_WIDTH; px++)
		{
			x_distance = std::abs(sCoord.x - px);
			y_distance = std::abs(sCoord.y - py);
			point_distance = (double)sqrt(x_distance*x_distance + y_distance*y_distance);
			if (point_distance <= radius)
			{
				int i = py * WINDOW_WIDTH + px;
				pixel_buffer[i] = color;
			}
		}
	}
}

void DrawTriangle(vertex vertex1, vertex vertex2, vertex vertex3, uint32_t color)
{
	vertex vertice_bottom;
	vertex vertice_middle;
	vertex vertice_top;
	
	// sort vertices by y
	vertice_bottom = vertex1;
	vertice_middle = vertex2;
	vertice_top = vertex3;

	if (vertice_bottom.y < vertice_middle.y)
	{
		swap_points2d(&vertice_bottom,&vertice_middle);
	}
	if (vertice_bottom.y < vertice_top.y )
	{
		swap_points2d(&vertice_bottom, &vertice_top);
	}
	if (vertice_middle.y < vertice_top.y)
	{
		swap_points2d(&vertice_middle, &vertice_top);
	}

	//		calculate slope				//
	// y= kx + b, k= delta(x)/ delta(y) //
	//									//

	int delta_X_top_bottom    = vertice_top.x - vertice_bottom.x;
	int delta_X_top_middle    = vertice_top.x - vertice_middle.x;
	int delta_X_middle_bottom = vertice_middle.x- vertice_bottom.x;

	int delta_Y_top_bottom    = vertice_top.y - vertice_bottom.y;
	int delta_Y_top_middle    = vertice_top.y - vertice_middle.y;
	int delta_Y_middle_bottom = vertice_middle.y - vertice_bottom.y;
	
	float slope_top_bottom = (delta_Y_top_bottom !=0) ? (float)delta_X_top_bottom / delta_Y_top_bottom : 0.0f;
	float slope_top_middle = (delta_Y_top_middle !=0)? (float)delta_X_top_middle / delta_Y_top_middle: 0.0f;
	float slope_middle_bottom = (delta_Y_middle_bottom != 0)? (float)delta_X_middle_bottom / delta_Y_middle_bottom : 0.0f;

	//									//
	// calculate shade slope -> dh/dy	//
	//									//
	float delta_H_top_bottom = vertice_top.h - vertice_bottom.h;
	float delta_H_top_middle = vertice_top.h - vertice_middle.h;
	float delta_H_middle_bottom = vertice_middle.h - vertice_bottom.h;

	float h_slope_top_bottom = (float)delta_H_top_bottom / delta_Y_top_bottom;
	float h_slope_top_middle = (float)delta_H_top_middle / delta_Y_top_middle;
	float h_slope_middle_bottom = (float)delta_H_middle_bottom / delta_Y_middle_bottom;

	//						//
	// Interpolate top-mid	//
	//						//
	int x0 = vertice_top.x;
	int y0 = vertice_top.y;
	float h0 = vertice_top.h;
	for (int y = vertice_top.y; y < vertice_middle.y; y++)
	{
		// x = x0 + (y - y0) * dx / dy;
		float x_left  = x0 + ( y - y0 ) * slope_top_middle;
		float x_right = x0 + ( y - y0 ) * slope_top_bottom;

		float h_left  = h0 + (y - y0) * h_slope_top_middle;
		float h_right = h0 + (y - y0) * h_slope_top_bottom;

		float x_start= x_left;
		float x_end= x_right;
		if (x_start > x_end)
		{
			int temp = x_start;
			x_start = x_end;
			x_end = temp;

			std::swap(h_left,h_right);
		}

		float dh_dx = 0.0f;
		float dx = (x_right - x_left);
		
		if (dx != 0.0f)
		{
			dh_dx= (h_right - h_left)/dx;
		}

		float current_h = h_left;
		
		for (float x = x_start; x < x_end; x++)
		{
			float intensity = current_h;
			if (x < 0 || x >= WINDOW_WIDTH ||y < 0 || y >= WINDOW_HEIGHT)
			{
				continue;
			}

			int i =(int)y * WINDOW_WIDTH + x;
			uint32_t final_color = ShadeColor(color,intensity);
			pixel_buffer[i] = final_color;
			current_h+= dh_dx;
		}		
	}

	//							//
	// Interpolate mid-bottom	//
	//							//
	int x0_m = vertice_middle.x;
	int y0_m = vertice_middle.y;
	float h0_m = vertice_middle.h;
	for (int y = y0_m; y < vertice_bottom.y; y++)
	{
		// x = x0 + (y - y0) * dx / dy;
		float x_left = x0_m + (y - y0_m) * slope_middle_bottom;
		float x_right = x0 + (y - y0) * slope_top_bottom;

		float h_left = h0_m + (y - y0_m) * h_slope_middle_bottom;
		float h_right = h0 + (y - y0) * h_slope_top_bottom;

		int x_start = x_left;
		int x_end = x_right;
		if (x_start > x_end)
		{
			int temp = x_start;
			x_start = x_end;
			x_end = temp;
		}

		float dh_dx = 0.0f;
		float dx = (x_right - x_left);

		if (dx != 0.0f)
		{
			dh_dx = (h_right-h_left) / dx;
		}

		float current_h = h_left;
		for (float x = x_start; x < x_end; x++)
		{
			float intensity = current_h;

			int i = (int)y * WINDOW_WIDTH + x;
			pixel_buffer[i] = ShadeColor(color, intensity);
			current_h += dh_dx;
		}
	}
}

void DrawLine(vertex point1, vertex point2, uint32_t color)
{
	vertex point_top = point1;
	vertex point_bottom = point2;

	if (point1.y > point2.y)
	{
		swap_points2d(&point_top, &point_bottom);
	}
	// calculate the slope
	int delta_x = point_bottom.x - point_top.x;
	int delta_y = point_bottom.y - point_top.y;
	float slope = (float) delta_x / (float) delta_y;

	if (delta_y == 0)
	{
		if (delta_x == 0)
		{
			return;
		}
				
		int	x_start = point_top.x;
		int x_end = point_bottom.x;
		if (x_start > x_end)
		{
			int temp = x_start;
			x_start = x_end;
			x_end = temp;
		}

		int y = point_top.y;
		for (int x = x_start ; x< x_end ; x++)
		{
			if(x>=0 && x < WINDOW_WIDTH && y>=0 && y < WINDOW_HEIGHT)
			{
				int i = y * WINDOW_WIDTH + x;
				pixel_buffer[i] = color;
			}
		}
		return;
	}

	for (int y = point_top.y; y < point_bottom.y; y++)
	{
		// x = x0 + (y - y0) * dx / dy;
		float x = point_top.x + (y - point_top.y) * slope;
		if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT)
		{
			int i = y * WINDOW_WIDTH + x;
			pixel_buffer[i] = color;
		}
	}
}

void DrawLineTriangle(vertex point1, vertex point2, vertex point3, uint32_t color)
{
	DrawLine(point1, point2, color);
	DrawLine(point2, point3, color);
	DrawLine(point3, point1, color);
}

// Take vertice & multiply by matrix
void VectorMatrixMul(const vertex &inputV, vertex &outputV,float (&matrix) [4][4])
{
	// matrix [row][col]
	outputV.x = inputV.x * matrix[0][0] + inputV.y * matrix[1][0] + inputV.z * matrix[2][0]+ matrix[3][0];
	outputV.y = inputV.x * matrix[0][1] + inputV.y * matrix[1][1] + inputV.z * matrix[2][1]+ matrix[3][1];
	outputV.z = inputV.x * matrix[0][2] + inputV.y * matrix[1][2] + inputV.z * matrix[2][2]+ matrix[3][2];
	outputV.h = inputV.h;
	float dev = inputV.x * matrix[0][3] + inputV.y * matrix[1][3] + inputV.z * matrix[2][3]+ matrix[3][3];

	// devide with z coord to match distance squezzing of object
	if (dev != 0)
	{
		outputV.x /= dev;
		outputV.y /= dev;
		outputV.z /= dev;
	}
}

void OnUserUpdate( float elapsedTime)
{
	std::fill(
		pixel_buffer,
		pixel_buffer + WINDOW_WIDTH * WINDOW_HEIGHT,
		//0x00000000
		0x00C0C0C0
	);

	uint32_t color = 0x00FF0000; // Red

	// make a cube and project it ?
	// cube -> triangles -> vertices
	model cube1 =
	{{ // vector <triangle>
		// SOUTH
		{{
			{0.0f, 0.0f, 0.0f, 1.0f},
			{0.0f, 1.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 0.0f, 1.0f}
		}},
		{{
			{0.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 0.0f, 1.0f},
			{1.0f, 0.0f, 0.0f, 1.0f}
		}},
		// EAST
		{{
			{1.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f, 1.0f}
		}},
		{{
			{1.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f, 1.0f},
			{1.0f, 0.0f, 1.0f, 1.0f}
		}},
		// WEST
		{{
			{0.0f, 0.0f, 0.0f, 1.0f},
			{0.0f, 1.0f, 1.0f, 1.0f},
			{0.0f, 1.0f, 0.0f, 1.0f}
		}},
		{{
			{0.0f, 0.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 1.0f, 1.0f},
			{0.0f, 1.0f, 1.0f, 1.0f}
		}},
		// NORTH
		{{
			{0.0f, 0.0f, 1.0f, 1.0f},
			{1.0f, 1.0f, 1.0f, 1.0f},
			{0.0f, 1.0f, 1.0f, 1.0f}
		}},
		{{
			{0.0f, 0.0f, 1.0f, 1.0f},
			{1.0f, 0.0f, 1.0f, 1.0f},
			{1.0f, 1.0f, 1.0f, 1.0f}
		}},
		// TOP
		{{
			{0.0f, 1.0f, 0.0f, 1.0f},
			{0.0f, 1.0f, 1.0f, 1.0f},
			{1.0f, 1.0f, 1.0f, 1.0f}
		}},
		{{
			{0.0f, 1.0f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f, 1.0f},
			{1.0f, 1.0f, 0.0f, 1.0f}
		}},
		// BOTTOM
		{{
			{0.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 0.0f, 1.0f, 1.0f},
			{0.0f, 0.0f, 1.0f, 1.0f}
		}},
		{{
			{0.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 0.0f, 1.0f, 1.0f}
		}}
		 
	}};

	model cube;
	cube.LoadObjFile("D:/source/Cub3/Monkey.obj");

	vertex ver1 = { 160, 100,50, 1.0 };
	vertex ver2 = { 110, 150,60, 0.5 };
	vertex ver3 = { 180, 200,70, 0.0 };

	triangle projTriangle = {};

	//DrawTriangle(ver1, ver2, ver3, color );
	
	Matrix4x4 m;
	Matrix4x4 matRotX;
	Matrix4x4 matRotZ;

	vector3D camera = { 0,0,0 };


	rotTheta += 0.3f * elapsedTime;

	// Rotation Z
	matRotZ.matrix[0][0] = cosf(rotTheta);
	matRotZ.matrix[0][1] = sinf(rotTheta);
	matRotZ.matrix[1][0] = -sinf(rotTheta);
	matRotZ.matrix[1][1] = cosf(rotTheta);
	matRotZ.matrix[2][2] = 1;
	matRotZ.matrix[3][3] = 1;

	// Rotation X
	matRotX.matrix[0][0] = 1;
	matRotX.matrix[1][1] = cosf(rotTheta * 0.5f);
	matRotX.matrix[1][2] = sinf(rotTheta * 0.5f);
	matRotX.matrix[2][1] = -sinf(rotTheta * 0.5f);
	matRotX.matrix[2][2] = cosf(rotTheta * 0.5f);
	matRotX.matrix[3][3] = 1;

	float zNear = 0.1f;
	float zFar = 1000.0f;
	float q1 = zFar / (zFar - zNear);
	float q2 = (zFar * zNear) / (zFar - zNear);

	float fovDegrees = 90.0f;
	float aspectRatio = (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT;
	float fFovRad = 1.0f / tanf( fovDegrees*(3.14159f / 180.0f) * 0.5f );
	
	float projectionValues[4][4] =
	{
		{ aspectRatio * fFovRad, 0 , 0, 0},
		{0, fFovRad, 0, 0},
		{0, 0, q1, 1},
		{0,0,-q2,0},
	};


	m.InitalizeMatrix(projectionValues);

	for (auto t : cube.triangles)
	{
		triangle tOffset, tRotatedZ, tRotatedZX;

		//rotate Z
		VectorMatrixMul(t.v[0], tRotatedZ.v[0], matRotZ.matrix);
		VectorMatrixMul(t.v[1], tRotatedZ.v[1], matRotZ.matrix);
		VectorMatrixMul(t.v[2], tRotatedZ.v[2], matRotZ.matrix);
		// rotate X
		VectorMatrixMul(tRotatedZ.v[0], tRotatedZX.v[0], matRotX.matrix);
		VectorMatrixMul(tRotatedZ.v[1], tRotatedZX.v[1], matRotX.matrix);
		VectorMatrixMul(tRotatedZ.v[2], tRotatedZX.v[2], matRotX.matrix);

		// make some offset from the camera
		tOffset = tRotatedZX;
		tOffset.v[0].z += 5.0f;
		tOffset.v[1].z += 5.0f;
		tOffset.v[2].z += 5.0f;

		vector3D vectorA;
		vector3D vectorB;
		vector3D normal;
		vector3D light = {0,0,-1}; // set behind the eyes (0,0,0)

		vectorA.x = tOffset.v[1].x - tOffset.v[0].x;
		vectorA.y = tOffset.v[1].y - tOffset.v[0].y;
		vectorA.z = tOffset.v[1].z - tOffset.v[0].z;
		
		vectorB.x = tOffset.v[2].x - tOffset.v[0].x;
		vectorB.y = tOffset.v[2].y - tOffset.v[0].y;
		vectorB.z = tOffset.v[2].z - tOffset.v[0].z;
		 
		normal.x = vectorA.y * vectorB.z - vectorA.z * vectorB.y;
		normal.y = vectorA.z * vectorB.x - vectorA.x * vectorB.z;
		normal.z = vectorA.x * vectorB.y - vectorA.y * vectorB.x;
		
		float normalSize = normal.CalculateSize();
		float lightSize= light.CalculateSize();

		normal.x /= normalSize;
		normal.y /= normalSize;
		normal.z /= normalSize;

		light.x /= lightSize;
		light.y /= lightSize;
		light.z /= lightSize;

		float cameraDotNormal = normal.x * (tOffset.v[1].x - camera.x)
			+ normal.y * (tOffset.v[1].y - camera.y)
			+ normal.z * (tOffset.v[1].z - camera.z);

		if (cameraDotNormal >= 0.0f)
		{
			continue;
		}

		float lightAngle= normal.x * light.x
			+ normal.y * light.y
			+ normal.z * light.z;

		// 3D -> 2D
		VectorMatrixMul(tOffset.v[0], projTriangle.v[0], m.matrix);
		VectorMatrixMul(tOffset.v[1], projTriangle.v[1], m.matrix);
		VectorMatrixMul(tOffset.v[2], projTriangle.v[2], m.matrix);
	
		// scale properly 
		float scaleZ = 0.4f;
		projTriangle.v[0].x+= 1.0f;
		projTriangle.v[0].y+= 1.0f;
		projTriangle.v[1].x+= 1.0f;
		projTriangle.v[1].y+= 1.0f;
		projTriangle.v[2].x+= 1.0f;
		projTriangle.v[2].y+= 1.0f;

		projTriangle.v[0].x*= scaleZ * (float)WINDOW_WIDTH;
		projTriangle.v[0].y*= scaleZ * (float)WINDOW_HEIGHT;
	
		projTriangle.v[1].x*= scaleZ * (float)WINDOW_WIDTH;
		projTriangle.v[1].y*= scaleZ * (float)WINDOW_HEIGHT;
	
		projTriangle.v[2].x*= scaleZ * (float)WINDOW_WIDTH;
		projTriangle.v[2].y*= scaleZ * (float)WINDOW_HEIGHT;
	
		//DrawLineTriangle(projTriangle.v[0], projTriangle.v[1], projTriangle.v[2], colors[2] * cameraDotNormal);
		// light up each side depending on surface angle towards the camera

		uint32_t color = ShadeColor(Red, lightAngle);
		DrawTriangle(projTriangle.v[0], projTriangle.v[1], projTriangle.v[2], color);
	}
}


int main()
{
	//printf("Hello there");
	
	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto previousTime = startTime;


	// Window buffer
	pixel_buffer = (uint32_t*)malloc(WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(uint32_t));

	//OnUserUpdate(elapsedTime);


	// Bitmap
	bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpi.bmiHeader.biWidth = WINDOW_WIDTH;
	bmpi.bmiHeader.biHeight = -WINDOW_HEIGHT; // Negative height = top-down
	bmpi.bmiHeader.biPlanes = 1;
	bmpi.bmiHeader.biBitCount = 32;
	bmpi.bmiHeader.biCompression = BI_RGB;

	// Register Window Class
	WNDCLASSEX wc;
	MSG Msg;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = windowClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	//Create the Window
	HWND hwnd;
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
		windowClassName,
		L"Cube++",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
		NULL, NULL, GetModuleHandle(NULL), NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);

	//while (GetMessage(&Msg, NULL, 0, 0) > 0)
	//{
	//	TranslateMessage(&Msg);
	//	DispatchMessage(&Msg);
	//}

	while (true)
	{
		// Process pending Windows messages
		MSG Msg;

		while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_QUIT)
			{
				return 0;
			}
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}

		// Measure frame time
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();

		float elapsedTime =
			std::chrono::duration<float>(
				currentTime - previousTime
			).count();

		previousTime = currentTime;

		// Update your application
		OnUserUpdate(elapsedTime); 

		InvalidateRect(hwnd, NULL, FALSE);
		UpdateWindow(hwnd);

		// Optional: limit CPU usage / target ~60 updates per second
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}


	return 0;
}