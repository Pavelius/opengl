/*
 *		This Code Was Created By Jeff Molofee 2000
 *		A HUGE Thanks To Fredric Echols For Cleaning Up
 *		And Optimizing This Code, Making It More Flexible!
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit My Site At nehe.gamedev.net
 *		Modified by Pavel Chistyakov
 */

#include "ogl.h"
#include "cface/file.h"
#include "formats/png.h"
#include "formats/utils.h"
#include <windows.h>			// Header File For Windows
#include <gl\gl.h>				// Header File For The OpenGL32 Library
#include <gl\glu.h>				// Header File For The GLu32 Library

static HDC			hDC = 0;	// Private GDI Device Context
static HGLRC		hRC = 0;	// Permanent Rendering Context
static HWND			hWnd = 0;	// Holds Our Window Handle
static HINSTANCE	hInstance;	// Holds The Instance Of The Application

bool				ogl::keys[256];	// Array Used For The Keyboard Routine
static bool			active = true;// Window Active Flag Set To TRUE By Default

void 				ogl_draw_scene();
int					ogl_init();
void				ogl_resize(int width, int height);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT	uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)								// Check For Windows Messages
	{
	case WM_ACTIVATE:							// Watch For Window Activate Message
		if(!HIWORD(wParam))						// Check Minimization State
			active = 1;							// Program Is Active
		else
			active = 0;							// Program Is No Longer Active
		return 0;								// Return To The Message Loop
	case WM_SYSCOMMAND:							// Intercept System Commands
		switch(wParam)							// Check System Calls
		{
		case SC_SCREENSAVE:						// Screensaver Trying To Start?
		case SC_MONITORPOWER:					// Monitor Trying To Enter Powersave?
			return 0;							// Prevent From Happening
		}
		break;									// Exit
	case WM_CLOSE:								// Did We Receive A Close Message?
		PostQuitMessage(0);						// Send A Quit Message
		return 0;								// Jump Back
	case WM_KEYDOWN:							// Is A Key Being Held Down?
		ogl::keys[wParam] = 1;						// If So, Mark It As TRUE
		return 0;								// Jump Back
	case WM_KEYUP:								// Has A Key Been Released?
		ogl::keys[wParam] = 0;						// If So, Mark It As FALSE
		return 0;								// Jump Back
	case WM_SIZE:								// Resize The OpenGL Window
		ogl_resize(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
		return 0;								// Jump Back
	}
	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

bool ogl::create(const char* title, int width, int height, unsigned char bits, bool fullscreen)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values

	WindowRect.left = 0;				// Set Left Value To 0
	WindowRect.right = width;			// Set Right Value To Requested Width
	WindowRect.top = 0;					// Set Top Value To 0
	WindowRect.bottom = height;			// Set Bottom Value To Requested Height

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if(!RegisterClass(&wc))									// Attempt To Register The Window Class
		return false;											// Return FALSE

	dwExStyle	= WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;		// Window Extended Style
	dwStyle		= WS_OVERLAPPEDWINDOW;						// Windows Style

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if(!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		destroy();									// Reset The Display
		return false;								// Return FALSE
	}

	static PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if(!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		destroy();									// Reset The Display
		return false;								// Return FALSE
	}

	if(!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		destroy();									// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		destroy();									// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;								// Return FALSE
	}

	if(!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		destroy();									// Reset The Display
		MessageBox(NULL,"Can't Create GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		destroy();									// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	return true;									// Success
}

void ogl::destroy()
{
	if(hRC)
	{
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(hRC);
		hRC = 0;
	}
	if(hDC)
	{
		ReleaseDC(hWnd,hDC);
		hDC = 0;
	}
	if(hWnd)
	{
		DestroyWindow(hWnd);
		hWnd = 0;
	}
	UnregisterClass("OpenGL", hInstance);
	hInstance = 0;
}

int ogl::input()
{
	MSG	msg;									// Windows Message Structure
	while(PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
	{
		if(msg.message==WM_QUIT)				// Have We Received A Quit Message?
			return 0;
		else									// If Not, Deal With Window Messages
		{
			TranslateMessage(&msg);				// Translate The Message
			DispatchMessage(&msg);				// Dispatch The Message
		}
	}
	if(active)									// Program Active?
		SwapBuffers(hDC);						// Swap Buffers (Double Buffering)
	return 1;
}

bool ogl::load(const char* url, unsigned* texture, int num)
{
	int size;
	unsigned w, h;
	void *p, *p1;
	p = io::file::load(url, &size);
	if(!p)
		return false;
	if(!formats::png::decode(&p1, &w, &h, p, size, formats::ColorRGB))
	{
		delete (char*)p;
		return false;
	}
	delete (char*)p;
	glGenTextures(num, texture);					// Create Three Textures
	switch(num)
	{
	case 1:
		// Typical Texture Generation Using Data From The Bitmap
		glBindTexture(GL_TEXTURE_2D, *texture);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, p1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case 3:
		// Create Nearest Filtered Texture
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, p1);
		// Create Linear Filtered Texture
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, p1);
		// Create MipMapped Texture
		glBindTexture(GL_TEXTURE_2D, texture[2]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w, h, GL_RGB, GL_UNSIGNED_BYTE, p1);
		break;
	}
	delete (char*)p1;
	return true;
}

int cface_main(int argc, char* argv[]) // Window Show State
{
	if(!ogl::create("OGL framework", 640, 480, 32))
		return 0;
	ogl_resize(640, 480);
	ogl_init();
	// Quit If Window Was Not Created
	while(true)
	{
		if(active)
			ogl_draw_scene();
		if(!ogl::input())
			break;
	}
	ogl::destroy();								// Kill The Window
	return 0;
}