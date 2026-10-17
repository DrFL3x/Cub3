#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <Windows.h>
#include <math.h>
#include <cmath>

int WINDOW_HEIGHT = 700;
int WINDOW_WIDTH  = 700;

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

void DrawTriangle(vertex point1, vertex point2, vertex point3, uint32_t color)
{

	vertex vertice_bottom;
	vertex vertice_middle;
	vertex vertice_top;
	
	// sort vertices by y
	vertice_bottom = point1;
	vertice_middle = point2;
	vertice_top = point3;

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

	// calculate slope
	// y= kx + b
	// k= delta(x)/ delta(y)

	int delta_X_top_bottom    = vertice_top.x - vertice_bottom.x;
	int delta_X_top_middle    = vertice_top.x - vertice_middle.x;
	int delta_X_middle_bottom = vertice_middle.x- vertice_bottom.x;

	int delta_Y_top_bottom    = vertice_top.y - vertice_bottom.y;
	int delta_Y_top_middle    = vertice_top.y - vertice_middle.y;
	int delta_Y_middle_bottom = vertice_middle.y - vertice_bottom.y;
	
	float slope_top_bottom    = (float)delta_X_top_bottom / delta_Y_top_bottom;
	float slope_top_middle    = (float)delta_X_top_middle / delta_Y_top_middle;
	float slope_middle_bottom = (float)delta_X_middle_bottom / delta_Y_middle_bottom;

	//
	// calculate shade slope -> dh/dy
	//
	float delta_H_top_bottom = vertice_top.h - vertice_bottom.h;
	float delta_H_top_middle = vertice_top.h - vertice_middle.h;
	float delta_H_middle_bottom = vertice_middle.h - vertice_bottom.h;

	float h_slope_top_bottom = (float)delta_H_top_bottom / delta_Y_top_bottom;
	float h_slope_top_middle = (float)delta_H_top_middle / delta_Y_top_middle;
	float h_slope_middle_bottom = (float)delta_H_middle_bottom / delta_Y_middle_bottom;

	// Interpolate top-mid 
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

		int x_start= x_left;
		int x_end= x_right;
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
			dh_dx= (h_right - h_left)/dx;
		}

		float current_h = h_left;
		
		for (float x = x_start; x < x_end; x++)
		{
			// extract color channels
			//   00000001 10101001 10011010 10101010  (32 - bit color number)
			// & 00000000 00000000 00000000 11111111  (0xFF mask)
			uint32_t r = (color >> 16) & 0xFF;
			uint32_t g = (color >> 8) & 0xFF;
			uint32_t b = color & 0xFF;

			float intensity = current_h;
			if (intensity < 0.0f) intensity = 0.0f;
			if (intensity > 1.0f) intensity = 1.0f;

			uint32_t shaded_r = (uint32_t)(r * intensity);
			uint32_t shaded_g = (uint32_t)(g * intensity);
			uint32_t shaded_b = (uint32_t)(b * intensity);

			int i =(int)y * WINDOW_WIDTH + x;
			uint32_t final_color = (shaded_r << 16) | (shaded_g << 8) | shaded_b;
			pixel_buffer[i] = final_color;
			current_h+= dh_dx;
		}		
	}
	// Interpolate mid-bottom
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
			uint32_t r = (color >> 16) & 0xFF;
			uint32_t g = (color >> 8) & 0xFF;
			uint32_t b = color & 0xFF;

			float intensity = current_h;
			if (intensity < 0.0f) intensity = 0.0f;
			if (intensity > 1.0f) intensity = 1.0f;

			uint32_t shaded_r = (uint32_t)(r * intensity);
			uint32_t shaded_g = (uint32_t)(g * intensity);
			uint32_t shaded_b = (uint32_t)(b * intensity);

			int i = (int)y * WINDOW_WIDTH + x;
			uint32_t final_color = (shaded_r << 16) | (shaded_g << 8) | shaded_b;
			pixel_buffer[i] = final_color;
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
	float dev = inputV.x * matrix[0][3] + inputV.y * matrix[1][3] + inputV.z * matrix[2][3]+ matrix[3][3];

	// devide with z coord to match distance squezzing of object
	if (dev != 0)
	{
		outputV.x /= dev;
		outputV.y /= dev;
		outputV.z /= dev;
	}
}

void OnUserUpdate()
{
	uint32_t color = 0x00FF0000; // Red

	vertex ver1 = { 160, 100,50, 1.0 };
	vertex ver2 = { 110, 150,60, 0.5};
	vertex ver3 = { 180, 200,70, 0.0 };

	vertex projVer1 = {};
	vertex projVer2 = {};
	vertex projVer3 = {};

	DrawLineTriangle(ver1, ver2, ver3, color);
	//DrawTriangle(ver1, ver2, ver3, color);
	Matrix4x4 m;

	int WINDOW_HEIGHT = 700;
	int WINDOW_WIDTH = 700;

	float zNear = 0.1f;
	float zFar = 1000.0f;
	float q1 = zFar / (zFar - zNear);
	float q2 = (zFar * zNear) / (zFar - zNear);

	float fovDegrees = 90.0f;
	float aspectRatio = (float)WINDOW_WIDTH/ (float)WINDOW_HEIGHT;
	float fFovRad = 1.0f / tanf( fovDegrees*(3.14159f / 180.0f) * 0.5f );
	
	float values[4][4] =
	{
		{ aspectRatio * fFovRad, 0 , 0, 0},
		{0, fFovRad, 0, 0},
		{0, 0, q1, 1},
		{0,0,-q2,0},
	};

	m.InitalizeMatrix(values);
	VectorMatrixMul(ver1, projVer1, m.matrix);
	VectorMatrixMul(ver2, projVer2, m.matrix);
	VectorMatrixMul(ver3, projVer3, m.matrix);

	projVer1.x += 1.0f; projVer1.y += 1.0f;
	projVer2.x += 1.0f; projVer2.y += 1.0f;
	projVer3.x += 1.0f; projVer3.y += 1.0f;
	//
	projVer1.x *= 0.1 * (float)WINDOW_WIDTH;
	projVer1.y *= 0.1 * (float)WINDOW_HEIGHT;
	projVer2.x *= 0.1 * (float)WINDOW_WIDTH;
	projVer2.y *= 0.1 * (float)WINDOW_HEIGHT;
	projVer3.x *= 0.1 * (float)WINDOW_WIDTH;
	projVer3.y *= 0.1 * (float)WINDOW_HEIGHT;


	DrawLineTriangle(projVer1, projVer2, projVer3, color);
}


int main()
{
	printf("Hello there");
	
	int rect_w = 50;
	int rect_h = 50;

	// Window buffer
	pixel_buffer = (uint32_t*)malloc(WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(uint32_t));

	vertex xy1 = {100,100,1.0};
	vertex xy2 = {220,150,0.5};
	vertex xy3 = {170,200,0.0};
	vertex xy1_l = {160,100};
	vertex xy2_l = {110,150};
	vertex xy3_l = {180,200};

	//DrawTriangle(xy1, xy2, xy3, color);

	OnUserUpdate();

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

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return 0;
}