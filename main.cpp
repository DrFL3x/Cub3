//        ______________
//       /             /|
//      /____________ / |
//     |  _________  |  |
//     | |         | |  | 
//     | |  🄿🄻🄰🅈  | |  /_    
//    /| |  .      | | / /| 
//   / | |_________| |/ / / 
//  /  |_______3++___| / /   
// /__________________/ /
// |  _+|            | /
// |_________________|/
//


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
#include <algorithm>

HDC hdc = NULL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND hwnd = NULL;

const wchar_t windowClassName[] = L"BisquitWindow";
uint32_t* pixel_buffer;
BITMAPINFO bmpi;

int WINDOW_HEIGHT = 800;
int WINDOW_WIDTH  = 800;

float rotTheta= 0.0f;
float xM;

struct Matrix4x4
{
	float matrix[4][4] = { 0 };
	
	// projects mesh 3d -> 2D
	void Project(Matrix4x4 &outMat ,float zNear, float zFar, float fovDegrees, float aspectRatio)
	{
	
		float q1 = zFar / (zFar - zNear);
		float q2 = (zFar * zNear) / (zFar - zNear);
	
		float fFovRad = 1.0f / tanf(fovDegrees * (3.14159f / 180.0f) * 0.5f);
		
		outMat.matrix[0][0] = aspectRatio * fFovRad;
		outMat.matrix[0][1] = 0.0f;
		outMat.matrix[0][2] = 0.0f;
		outMat.matrix[0][3] = 0.0f;

		outMat.matrix[1][0] = 0.0f;
		outMat.matrix[1][1] = fFovRad;
		outMat.matrix[1][2] = 0.0f;
		outMat.matrix[1][3] = 0.0f;

		outMat.matrix[2][0] = 0.0f;
		outMat.matrix[2][1] = 0.0f;
		outMat.matrix[2][2] = q1;
		outMat.matrix[2][3] = 1.0f; // Perspective w-divider flag

		outMat.matrix[3][0] = 0.0f;
		outMat.matrix[3][1] = 0.0f;
		outMat.matrix[3][2] = -q2;
		outMat.matrix[3][3] = 0.0f;

	}

	// Z-axis mesh rotation
	void RotateZ(Matrix4x4 &outMat,float rotTheta)
	{
		outMat.matrix[0][0] = cosf(rotTheta);
		outMat.matrix[0][1] = sinf(rotTheta);
		outMat.matrix[1][0] = -sinf(rotTheta);
		outMat.matrix[1][1] = cosf(rotTheta);
		outMat.matrix[2][2] = 1;
		outMat.matrix[3][3] = 1;
	}
	// X-axis mesh rotation
	void RotateX(Matrix4x4 &outMat, float rotTheta)
	{
		outMat.matrix[0][0] = 1;
		outMat.matrix[1][1] = cosf(rotTheta * 0.5f);
		outMat.matrix[1][2] = sinf(rotTheta * 0.5f);
		outMat.matrix[2][1] = -sinf(rotTheta * 0.5f);
		outMat.matrix[2][2] = cosf(rotTheta * 0.5f);
		outMat.matrix[3][3] = 1;
	}

	void RotateY(Matrix4x4& outMat, float rotTheta)
	{
		outMat.matrix[0][0] = cosf(rotTheta * 0.5f);
		outMat.matrix[0][2] = -sinf(rotTheta * 0.5f);
		outMat.matrix[1][1] = 1;
		outMat.matrix[2][0] = sinf(rotTheta * 0.5f);
		outMat.matrix[2][2] = cosf(rotTheta * 0.5f);
		outMat.matrix[3][3] = 1;
	}

	void CreateIdentity(Matrix4x4 &out)
	{
		out.matrix[0][0] = 1.0f;
		out.matrix[1][1] = 1.0f;
		out.matrix[2][2] = 1.0f;
		out.matrix[3][3] = 1.0f;
	}

	// Multiply 2 matrices
	void Multiply(Matrix4x4 &m1, Matrix4x4 &m2, Matrix4x4 &out)
	{
		for (int i = 0; i < 4; i++) // Rows
		{
			for (int j = 0; j < 4; j++)
			{
				out.matrix[i][j] =
				(m1.matrix[i][0] * m2.matrix[0][j]) +
				(m1.matrix[i][1] * m2.matrix[1][j]) +
				(m1.matrix[i][2] * m2.matrix[2][j]) +
				(m1.matrix[i][3] * m2.matrix[3][j]);
			}
		}
	}

	void Translate(Matrix4x4 &m ,float x, float y, float z)
	{
		m.matrix[0][0] = 1.0f;
		m.matrix[1][1] = 1.0f;
		m.matrix[2][2] = 1.0f;
		m.matrix[3][3] = 1.0f;
		m.matrix[3][0] = x;
		m.matrix[3][1] = y;
		m.matrix[3][2] = z;
	}

};

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

	void Normalize(vector3D& vector)
	{
		float vectorSize = vector.CalculateSize();
		vector.x /= vectorSize;
		vector.y /= vectorSize;
		vector.z /= vectorSize;
	}

	float GetDotProduct(const vector3D& v1, const  vector3D& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

};

typedef struct {
	float x;
	float y;
	float z;
	float h;
}vertex;

typedef struct
{
	vertex v[3];
	float color;
}triangle;

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

			if (line.length() >= 2 && line[0] == 'v' && line[1] == ' ')
			{
				vertex v;
				s >> junk >> v.x >> v.y >> v.z;
				v.h = 1.0f; // -> is this configurable in Blender ?
				verts.push_back(v);
			}

			if (line.length() >= 2 && line[0] == 'f' && line[1] == ' ')
			{
				s >> junk; 
				std::string token;
				int f_idx[3];
				bool validFace = true;

				for (int i = 0; i < 3; ++i)
				{
					if (!(s >> token)) 
					{
						validFace = false;
						break;
					}

					size_t slashPos = token.find('/');
					std::string indexStr = (slashPos != std::string::npos) ? token.substr(0, slashPos) : token;

					int idx = std::stoi(indexStr) - 1;

					if (idx >= 0 && idx < verts.size()) 
					{
						f_idx[i] = idx;
					}
					else 
					{
						validFace = false;
						break;
					}
				}

				if (validFace) 
				{
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

bool InitializeEngine(int width, int height)
{
	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;

	// window pixel buffer
	pixel_buffer = (uint32_t*)malloc(WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(uint32_t));
	if (!pixel_buffer) return false;

	// Bitmap info
	bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpi.bmiHeader.biWidth = WINDOW_WIDTH;
	bmpi.bmiHeader.biHeight = -WINDOW_HEIGHT; // Negative height = top-down
	bmpi.bmiHeader.biPlanes = 1;
	bmpi.bmiHeader.biBitCount = 32;
	bmpi.bmiHeader.biCompression = BI_RGB;

	return true;
}

bool InitializeWindow(HINSTANCE hInstance, int width, int height, const wchar_t* windowTitle)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(WHITE_BRUSH);
	wc.lpszClassName = windowClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL, NULL, hInstance, NULL
	);

	if (hwnd == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);
	return true;
}

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

void Swap(vertex* point1, vertex*  point2)
{
	vertex temp = *point1;
	*point1 = *point2;
	*point2 = temp;
}

uint32_t ShadeColor(uint32_t color, float intensity)
{
	// extract color channels
	//   00000001 10101001 10011010 10101010  (32 - bit color)
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
	for (float py = sCoord.y; py < sCoord.y + h; py++)
	{
		for (float px = sCoord.x; px < sCoord.x + w; px++)
		{
			int i = py * WINDOW_WIDTH + px;
			pixel_buffer[i] = color; // *(pixel_buffer + i) = color
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
		Swap(&vertice_bottom,&vertice_middle);
	}
	if (vertice_bottom.y < vertice_top.y )
	{
		Swap(&vertice_bottom, &vertice_top);
	}
	if (vertice_middle.y < vertice_top.y)
	{
		Swap(&vertice_middle, &vertice_top);
	}

	//		calculate slope				//
	// y= kx + b, k= delta(x)/ delta(y) //
	//									//

	int delta_X_top_bottom    = (int)vertice_top.x - vertice_bottom.x;
	int delta_X_top_middle    = (int)vertice_top.x - vertice_middle.x;
	int delta_X_middle_bottom = (int)vertice_middle.x- vertice_bottom.x;

	int delta_Y_top_bottom    = (int)vertice_top.y - vertice_bottom.y;
	int delta_Y_top_middle    = (int)vertice_top.y - vertice_middle.y;
	int delta_Y_middle_bottom = (int)vertice_middle.y - vertice_bottom.y;
	
	float slope_top_bottom	  =	(delta_Y_top_bottom !=0) ? (float)delta_X_top_bottom / delta_Y_top_bottom : 0.0f;
	float slope_top_middle    =	(delta_Y_top_middle !=0)? (float)delta_X_top_middle / delta_Y_top_middle: 0.0f;
	float slope_middle_bottom = (delta_Y_middle_bottom != 0)? (float)delta_X_middle_bottom / delta_Y_middle_bottom : 0.0f;

	//									//
	// calculate shade slope -> dh/dy	//
	//									//
	int delta_H_top_bottom =	(int)vertice_top.h - vertice_bottom.h;
	int delta_H_top_middle =	(int)vertice_top.h - vertice_middle.h;
	int delta_H_middle_bottom = (int)vertice_middle.h - vertice_bottom.h;

	float h_slope_top_bottom	= (delta_Y_top_bottom != 0)?  (float)delta_H_top_bottom		/ delta_Y_top_bottom : 0.0f;
	float h_slope_top_middle    = (delta_Y_top_middle !=0)?   (float)delta_H_top_middle		/ delta_Y_top_middle : 0.0f;
	float h_slope_middle_bottom = (delta_Y_middle_bottom!=0)? (float)delta_H_middle_bottom	/ delta_Y_middle_bottom : 0.0f;

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
			if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT)
			{
				continue;
			}

			float intensity = current_h;
			int i =(int)y * WINDOW_WIDTH + x;

			pixel_buffer[i] = ShadeColor(color, intensity);
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

			std::swap(h_left, h_right);
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
			if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT)
			{
				continue;
			}

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
		Swap(&point_top, &point_bottom);
	}
	// calculate the slope
	int delta_x = point_bottom.x - point_top.x;
	int delta_y = point_bottom.y - point_top.y;

	float slope = (delta_y!=0)? (float) delta_x / (float) delta_y : 0.0f;

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

void ClearScreen()
{
	std::fill(
		pixel_buffer,
		pixel_buffer + WINDOW_WIDTH * WINDOW_HEIGHT,
		0x00C0C0C0
	);
}

void OnUserUpdate( float elapsedTime)
{
	Matrix4x4 mProj;
	Matrix4x4 matRotX;
	Matrix4x4 matRotY;
	Matrix4x4 matRotZ;
	Matrix4x4 matTrans;
	Matrix4x4 matView;
	triangle projTriangle = {};


	// clear window screen
	ClearScreen();

	// Coord sys is determined
	// by Blender.obj settings, i.e.
	// Forward Axis: -Z
	model model;
	model.LoadObjFile("D:/source/Cub3/Monkey.obj");
	
	vector3D camera = { 0,0,0 };

	// live rotate the model
	rotTheta += 0.3f * elapsedTime;

	matRotZ.RotateZ(matRotZ, rotTheta);
	matRotX.RotateX(matRotX, rotTheta);
	matRotY.RotateY(matRotY,rotTheta);
	matView.CreateIdentity(matView);

	float zNear = 0.1f;
	float zFar = 1000.0f;
	float fovDegrees = 90.0f;
	float aspectRatio = (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT;
	
	mProj.Project(mProj,zNear, zFar, fovDegrees, aspectRatio);
	matView.Multiply(matRotZ, matRotX, matView);

	// make some offset from the camera
	matTrans.Translate(matTrans,0.0f,0.0f,3.0f);
	matView.Multiply(matView, matTrans, matView);

	std::vector <triangle> renderTriangles;

	for (auto t : model.triangles)
	{
		triangle triangle;

		VectorMatrixMul(t.v[0], triangle.v[0], matView.matrix);
		VectorMatrixMul(t.v[1], triangle.v[1], matView.matrix);
		VectorMatrixMul(t.v[2], triangle.v[2], matView.matrix);

		vector3D vectorA;
		vector3D vectorB;
		vector3D normal;
		vector3D light = {0,0,-1}; // set behind the camera

		vectorA.x = triangle.v[1].x - triangle.v[0].x;
		vectorA.y = triangle.v[1].y - triangle.v[0].y;
		vectorA.z = triangle.v[1].z - triangle.v[0].z;
		
		vectorB.x = triangle.v[2].x - triangle.v[0].x;
		vectorB.y = triangle.v[2].y - triangle.v[0].y;
		vectorB.z = triangle.v[2].z - triangle.v[0].z;
		 
		normal.x = vectorA.y * vectorB.z - vectorA.z * vectorB.y;
		normal.y = vectorA.z * vectorB.x - vectorA.x * vectorB.z;
		normal.z = vectorA.x * vectorB.y - vectorA.y * vectorB.x;
		
		normal.Normalize(normal);
		light.Normalize(light);


		// check if camera direction & triangle normal align
		// hide triangle if not
		vector3D camDir =
		{
			triangle.v[1].x - camera.x,
			triangle.v[1].y - camera.y,
			triangle.v[1].z - camera.z
		};
			 
		float normalAlignment = camDir.GetDotProduct(normal, camDir);
		if (normalAlignment >= 0.0f)
		{
			continue;
		}

		float lightAngle = normal.GetDotProduct(normal,light);
		
		// 3D -> 2D
		VectorMatrixMul(triangle.v[0], projTriangle.v[0], mProj.matrix);
		VectorMatrixMul(triangle.v[1], projTriangle.v[1], mProj.matrix);
		VectorMatrixMul(triangle.v[2], projTriangle.v[2], mProj.matrix);
	
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
	

		// light up each side depending on surface angle towards the camera
		projTriangle.color = ShadeColor(Red, lightAngle);
		
		renderTriangles.push_back(projTriangle);
	}

	// sort triangles by z
	std::sort(renderTriangles.begin(), renderTriangles.end(), [](const triangle &t1, const triangle &t2)
		{
			float z1Avg = (t1.v[0].z + t1.v[1].z + t1.v[2].z)	/ 3.0f;
			float z2Avg = (t2.v[0].z + t2.v[1].z + t2.v[2].z)	/ 3.0f;
			return z1Avg > z2Avg; // Paint closer first
		}
	);

	for (auto tri : renderTriangles)
	{
		DrawTriangle(tri.v[0], tri.v[1], tri.v[2], tri.color);
	}

}

int main()
{
	int WIDTH = 800;
	int HEIGHT = 800;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	
	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto previousTime = startTime;

	if (!InitializeEngine(800, 800))
	{
		return -1; // bad
	}
	
	if (!InitializeWindow(hInstance, WIDTH, HEIGHT, L"Bisquit"))
	{
		return -1;
	}
	
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

	}

	if (pixel_buffer)
	{
		free(pixel_buffer);
		pixel_buffer = nullptr;
	}

	return 0;
}