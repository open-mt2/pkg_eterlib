#include "StdAfx.h"
#include "MSWindow.h"

#ifdef _WIN32
#include <windowsx.h>
#endif

#if defined(OPENMT2_PLATFORM_LINUX)
#include <SDL2/SDL.h>
#include <GL/gl.h>
SDL_Window* CMSWindow::ms_pSDLWindow = NULL;
SDL_Renderer* CMSWindow::ms_pSDLRenderer = NULL;
SDL_GLContext CMSWindow::ms_pGLContext = NULL;
#endif

CMSWindow::TWindowClassSet CMSWindow::ms_stWCSet;
HINSTANCE CMSWindow::ms_hInstance = NULL;

LRESULT CALLBACK MSWindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{	
	CMSWindow * pWnd = (CMSWindow *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (pWnd)
		return pWnd->WindowProcedure(hWnd, uiMsg, wParam, lParam);	

	return DefWindowProc(hWnd, uiMsg, wParam, lParam);
}

LRESULT CMSWindow::WindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
		case WM_SIZE:
			OnSize(wParam, lParam);
			break;

		case WM_ACTIVATEAPP:
			m_isActive = (wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE);
			break;
	}

	return DefWindowProc(hWnd, uiMsg, wParam, lParam);
}

void CMSWindow::OnSize(WPARAM wParam, LPARAM /*lParam*/)
{
	if (wParam == SIZE_MINIMIZED) 
	{
		InvalidateRect(m_hWnd, NULL, true);
		m_isActive = false;        
		m_isVisible = false;
	}
	else
	{
		m_isActive = true;
		m_isVisible = true;
	}
}

void CMSWindow::Destroy()
{
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pGLContext)
	{
		SDL_GL_DeleteContext(ms_pGLContext);
		ms_pGLContext = NULL;
	}
	if (ms_pSDLRenderer)
	{
		SDL_DestroyRenderer(ms_pSDLRenderer);
		ms_pSDLRenderer = NULL;
	}
	if (ms_pSDLWindow)
	{
		SDL_DestroyWindow(ms_pSDLWindow);
		ms_pSDLWindow = NULL;
	}
#endif
	if (!m_hWnd)
		return;

	if (IsWindow(m_hWnd))
		DestroyWindow(m_hWnd);
	
	m_hWnd = NULL;
	m_isVisible = false;
}

bool CMSWindow::Create(const char* c_szName, int brush, DWORD cs, DWORD ws, HICON hIcon, int iCursorResource, int width, int height)
{
	//assert(ms_hInstance != NULL);
	Destroy();

#if defined(OPENMT2_PLATFORM_LINUX)
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
		{
			TraceError("CMSWindow::Create: SDL_Init failed: %s", SDL_GetError());
			return false;
		}
	}

	if (width <= 0 || height <= 0)
	{
		Platform_GetWindowResolution(&width, &height);
		if (width <= 0) width = 1024;
		if (height <= 0) height = 768;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Force floating mode on Linux window managers (X11 & Wayland):
	// Do NOT specify SDL_WINDOW_RESIZABLE; set min/max size equal to target dimensions.
	// Tiling WMs (i3, Sway, Hyprland, bspwm) automatically treat fixed-size windows as floating.
	ms_pSDLWindow = SDL_CreateWindow(
		(c_szName && c_szName[0]) ? c_szName : "Metin2",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);

	if (!ms_pSDLWindow)
	{
		TraceError("CMSWindow::Create: SDL_CreateWindow failed: %s", SDL_GetError());
		return false;
	}

	SDL_SetWindowResizable(ms_pSDLWindow, SDL_FALSE);
	SDL_SetWindowMinimumSize(ms_pSDLWindow, width, height);
	SDL_SetWindowMaximumSize(ms_pSDLWindow, width, height);
	SDL_SetWindowSize(ms_pSDLWindow, width, height);
	SDL_SetWindowPosition(ms_pSDLWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	ms_pGLContext = SDL_GL_CreateContext(ms_pSDLWindow);
	if (!ms_pGLContext)
	{
		TraceError("[SDL2] SDL_GL_CreateContext failed: %s\n", SDL_GetError());
	}
	else
	{
		SDL_GL_MakeCurrent(ms_pSDLWindow, ms_pGLContext);
		SDL_GL_SetSwapInterval(1);

		typedef void (*PFNGLCLIPCONTROLPROC)(GLenum origin, GLenum depth);
		PFNGLCLIPCONTROLPROC pfnClipControl = (PFNGLCLIPCONTROLPROC)SDL_GL_GetProcAddress("glClipControl");
		if (pfnClipControl)
		{
			// Direct3D 9 depth range [0, 1] natively handled by OpenGL clipping
			pfnClipControl(GL_LOWER_LEFT, 0x935F /* GL_ZERO_TO_ONE */);
			Tracef("[SDL2] glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE) enabled\n");
		}

		glClearColor(0.094f, 0.11f, 0.14f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		SDL_GL_SwapWindow(ms_pSDLWindow);
		Tracef("[SDL2] Initialized OpenGL context for window %p\n", ms_pSDLWindow);
	}

	m_hWnd = (HWND)ms_pSDLWindow;
	m_isActive = true;
	m_isVisible = true;
	Platform_SetWindowResolution(width, height);
	Platform_SetWarpMouseCallback([](int x, int y) {
		if (ms_pSDLWindow) {
			SDL_WarpMouseInWindow(ms_pSDLWindow, x, y);
		}
	});
	SetRect(&m_rect, 0, 0, width, height);

	Tracef("[SDL2] Created native game window (floating): %p ('%s', %dx%d, video driver: %s)\n",
		ms_pSDLWindow, c_szName, width, height, SDL_GetCurrentVideoDriver());
	return true;
#else
	const char* c_szClassName = RegisterWindowClass(cs, brush, MSWindowProcedure, hIcon, iCursorResource);

	m_hWnd = CreateWindow(
						c_szClassName,
						c_szName,
						ws, 
						0, 0, 0, 0, 
						NULL,
						NULL, 
						ms_hInstance,
						NULL);

	if (!m_hWnd)
		return false;

	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR) this);
	//DestroyWindow(ImmGetDefaultIMEWnd(m_hWnd));

	return true;
#endif
}

void CMSWindow::SetVisibleMode(bool isVisible)
{
	m_isVisible = isVisible;

#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow)
	{
		if (isVisible)
			SDL_ShowWindow(ms_pSDLWindow);
		else
			SDL_HideWindow(ms_pSDLWindow);
	}
#else
	if (m_isVisible)
	{
		ShowWindow(m_hWnd, SW_SHOW);		
	}
	else
	{
		ShowWindow(m_hWnd, SW_HIDE);
	}	
#endif
}

void CMSWindow::Show()
{
	m_isVisible = true;
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow)
		SDL_ShowWindow(ms_pSDLWindow);
#else
	ShowWindow(m_hWnd, SW_SHOW);
#endif
}

void CMSWindow::Hide()
{
	m_isVisible = false;
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow)
		SDL_HideWindow(ms_pSDLWindow);
#else
	ShowWindow(m_hWnd, SW_HIDE);
#endif
}

bool CMSWindow::IsVisible()
{
	return m_isVisible;
}

bool CMSWindow::IsActive()
{
	return m_isActive;
}

HINSTANCE CMSWindow::GetInstance()
{
	return ms_hInstance;
}

HWND CMSWindow::GetWindowHandle()
{
	return m_hWnd;
}

int	CMSWindow::GetScreenWidth()
{
#if defined(OPENMT2_PLATFORM_LINUX)
	SDL_DisplayMode dm;
	if (SDL_GetCurrentDisplayMode(0, &dm) == 0)
		return dm.w;
#endif
	return GetSystemMetrics(SM_CXSCREEN);
}

int	CMSWindow::GetScreenHeight()
{
#if defined(OPENMT2_PLATFORM_LINUX)
	SDL_DisplayMode dm;
	if (SDL_GetCurrentDisplayMode(0, &dm) == 0)
		return dm.h;
#endif
	return GetSystemMetrics(SM_CYSCREEN);
}

void CMSWindow::GetWindowRect(RECT* prc)
{
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow && prc)
	{
		int x = 0, y = 0, w = 0, h = 0;
		SDL_GetWindowPosition(ms_pSDLWindow, &x, &y);
		SDL_GetWindowSize(ms_pSDLWindow, &w, &h);
		prc->left = x;
		prc->top = y;
		prc->right = x + w;
		prc->bottom = y + h;
		return;
	}
#endif
	::GetWindowRect(m_hWnd, prc);
}

void CMSWindow::GetClientRect(RECT* prc)
{
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow && prc)
	{
		int w = 0, h = 0;
		SDL_GetWindowSize(ms_pSDLWindow, &w, &h);
		prc->left = 0;
		prc->top = 0;
		prc->right = w;
		prc->bottom = h;
		return;
	}
#endif
	::GetClientRect(m_hWnd, prc);
}

void CMSWindow::GetMousePosition(POINT* ppt)
{
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ppt)
	{
		int x = 0, y = 0;
		SDL_GetMouseState(&x, &y);
		ppt->x = x;
		ppt->y = y;
		return;
	}
#endif
	GetCursorPos(ppt);
	ScreenToClient(m_hWnd, ppt);
}

void CMSWindow::SetPosition(int x, int y)
{
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow)
		SDL_SetWindowPosition(ms_pSDLWindow, x, y);
#else
	SetWindowPos(m_hWnd, NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
#endif
}

void CMSWindow::SetCenterPosition()
{
	RECT rc;

	GetClientRect(&rc);

	int windowWidth = rc.right - rc.left;
	int windowHeight = rc.bottom - rc.top;

	SetPosition((GetScreenWidth()-windowWidth)/2, (GetScreenHeight()-windowHeight)/2);
}

void CMSWindow::AdjustSize(int width, int height)
{
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow)
	{
		SDL_SetWindowResizable(ms_pSDLWindow, SDL_FALSE);
		SDL_SetWindowMinimumSize(ms_pSDLWindow, width, height);
		SDL_SetWindowMaximumSize(ms_pSDLWindow, width, height);
		SDL_SetWindowSize(ms_pSDLWindow, width, height);
		SDL_SetWindowPosition(ms_pSDLWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
	Platform_SetWindowResolution(width, height);
	SetRect(&m_rect, 0, 0, width, height);
#else
	SetRect(&m_rect, 0, 0, width, height);

	AdjustWindowRectEx(&m_rect,
						GetWindowStyle(m_hWnd),     
						GetMenu(m_hWnd ) != NULL,    
						GetWindowExStyle(m_hWnd ) ); 

	MoveWindow
	( 
		m_hWnd, 
		0, 
		0, 
		m_rect.right - m_rect.left, 
		m_rect.bottom - m_rect.top, 
		FALSE
	);
#endif
}

void CMSWindow::SetText(const char* c_szText)
{
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow && c_szText)
		SDL_SetWindowTitle(ms_pSDLWindow, c_szText);
#else
	SetWindowText(m_hWnd, c_szText);
#endif
}

void CMSWindow::SetSize(int width, int height)
{	
#if defined(OPENMT2_PLATFORM_LINUX)
	if (ms_pSDLWindow)
	{
		SDL_SetWindowResizable(ms_pSDLWindow, SDL_FALSE);
		SDL_SetWindowMinimumSize(ms_pSDLWindow, width, height);
		SDL_SetWindowMaximumSize(ms_pSDLWindow, width, height);
		SDL_SetWindowSize(ms_pSDLWindow, width, height);
		SDL_SetWindowPosition(ms_pSDLWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
	Platform_SetWindowResolution(width, height);
	SetRect(&m_rect, 0, 0, width, height);
#else
	SetWindowPos(m_hWnd, NULL, 0, 0, width, height, SWP_NOZORDER|SWP_NOMOVE);
#endif
}

const char * CMSWindow::RegisterWindowClass(DWORD style, int brush, WNDPROC pfnWndProc, HICON hIcon, int iCursorResource)
{
	char szClassName[1024];
	sprintf(szClassName, "eter - s%x:b%x:p:%zx", (unsigned int)style, brush, (size_t) pfnWndProc);

	TWindowClassSet::iterator f = ms_stWCSet.find((char*) szClassName);

	if (f != ms_stWCSet.end())
		return *f;

	const char* c_szStaticClassName = stl_static_string(szClassName).c_str();

	ms_stWCSet.insert((char * const) c_szStaticClassName);
	
	WNDCLASS wc;

	wc.style			= 0;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.lpfnWndProc		= pfnWndProc;
	wc.hCursor			= LoadCursor(ms_hInstance, MAKEINTRESOURCE(iCursorResource));
	wc.hIcon			= hIcon ? hIcon : LoadIcon(ms_hInstance, IDI_APPLICATION);
	wc.hbrBackground	= (HBRUSH) GetStockObject(brush);
	wc.hInstance		= ms_hInstance;	
	wc.lpszClassName	= c_szStaticClassName;
	wc.lpszMenuName		= "";

	if (!RegisterClass(&wc)) 
		return "";

	return c_szStaticClassName;
}

CMSWindow::CMSWindow()
{
	m_hWnd=NULL;
	m_isVisible=false;
#ifdef OPENMT2_PLATFORM_LINUX
	m_isActive=true;
#else
	m_isActive=false;
#endif
}

CMSWindow::~CMSWindow()
{
}