#include "StdAfx.h"
#include "MSApplication.h"

#if defined(__LINUX__)
#include <SDL2/SDL.h>

#ifndef MAKELPARAM
#define MAKELPARAM(l, h) ((LPARAM)(((WORD)(((DWORD_PTR)(l)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(h)) & 0xffff))) << 16))
#endif
#ifndef MAKEWPARAM
#define MAKEWPARAM(l, h) ((WPARAM)(((WORD)(((DWORD_PTR)(l)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(h)) & 0xffff))) << 16))
#endif

static WPARAM SDLKeycodeToVK(SDL_Keycode key)
{
	switch (key)
	{
		case SDLK_ESCAPE: return VK_ESCAPE;
		case SDLK_RETURN:
		case SDLK_KP_ENTER: return VK_RETURN;
		case SDLK_TAB: return VK_TAB;
		case SDLK_BACKSPACE: return VK_BACK;
		case SDLK_SPACE: return VK_SPACE;
		case SDLK_UP: return VK_UP;
		case SDLK_DOWN: return VK_DOWN;
		case SDLK_LEFT: return VK_LEFT;
		case SDLK_RIGHT: return VK_RIGHT;
		case SDLK_LSHIFT:
		case SDLK_RSHIFT: return VK_SHIFT;
		case SDLK_LCTRL:
		case SDLK_RCTRL: return VK_CONTROL;
		case SDLK_LALT:
		case SDLK_RALT: return VK_MENU;
		case SDLK_F1: return VK_F1;
		case SDLK_F2: return VK_F2;
		case SDLK_F3: return VK_F3;
		case SDLK_F4: return VK_F4;
		case SDLK_F5: return VK_F5;
		case SDLK_F6: return VK_F6;
		case SDLK_F7: return VK_F7;
		case SDLK_F8: return VK_F8;
		case SDLK_F9: return VK_F9;
		case SDLK_F10: return VK_F10;
		case SDLK_F11: return VK_F11;
		case SDLK_F12: return VK_F12;
		default:
			if (key >= SDLK_a && key <= SDLK_z)
				return 'A' + (key - SDLK_a);
			if (key >= SDLK_0 && key <= SDLK_9)
				return '0' + (key - SDLK_0);
			return (WPARAM)key;
	}
}
#endif

CMSApplication::CMSApplication()
{
}

CMSApplication::~CMSApplication()
{
//	for (TWindowClassSet::iterator i=ms_stWCSet.begin(); i!=ms_stWCSet.end(); ++i)
//		UnregisterClass(*i, ms_hInstance);
}

void CMSApplication::Initialize(HINSTANCE hInstance)
{
	ms_hInstance = hInstance;
}

void CMSApplication::MessageLoop()
{
	while (MessageProcess());
}

bool CMSApplication::IsMessage()
{
#if defined(__LINUX__)
	SDL_PumpEvents();
	return (SDL_PeepEvents(NULL, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0);
#else
	MSG msg;

	if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		return false;

	return true;
#endif
}

bool CMSApplication::MessageProcess()
{
#if defined(__LINUX__)
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
		{
			Tracef("[SDL2] Received SDL_QUIT event. Exiting MessageProcess.\n");
			PostQuitMessage(0);
			return false;
		}

		if (event.type == SDL_WINDOWEVENT)
		{
			if (event.window.event == SDL_WINDOWEVENT_CLOSE)
			{
				Tracef("[SDL2] Received SDL_WINDOWEVENT_CLOSE event. Exiting MessageProcess.\n");
				PostQuitMessage(0);
				return false;
			}
			else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				OnSize(SIZE_RESTORED, MAKELPARAM(event.window.data1, event.window.data2));
				WindowProcedure(m_hWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(event.window.data1, event.window.data2));
			}
			else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			{
				m_isActive = true;
				WindowProcedure(m_hWnd, WM_ACTIVATEAPP, WA_ACTIVE, 0);
			}
			else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				m_isActive = false;
				WindowProcedure(m_hWnd, WM_ACTIVATEAPP, WA_INACTIVE, 0);
			}
		}
		else if (event.type == SDL_MOUSEMOTION)
		{
			Platform_SetCursorPos(event.motion.x, event.motion.y);
			WindowProcedure(m_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(event.motion.x, event.motion.y));
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN)
		{
			Platform_SetCursorPos(event.button.x, event.button.y);
			UINT uMsg = WM_LBUTTONDOWN;
			if (event.button.button == SDL_BUTTON_RIGHT) uMsg = WM_RBUTTONDOWN;
			else if (event.button.button == SDL_BUTTON_MIDDLE) uMsg = WM_MBUTTONDOWN;
			WindowProcedure(m_hWnd, uMsg, 0, MAKELPARAM(event.button.x, event.button.y));
		}
		else if (event.type == SDL_MOUSEBUTTONUP)
		{
			Platform_SetCursorPos(event.button.x, event.button.y);
			UINT uMsg = WM_LBUTTONUP;
			if (event.button.button == SDL_BUTTON_RIGHT) uMsg = WM_RBUTTONUP;
			else if (event.button.button == SDL_BUTTON_MIDDLE) uMsg = WM_MBUTTONUP;
			WindowProcedure(m_hWnd, uMsg, 0, MAKELPARAM(event.button.x, event.button.y));
		}
		else if (event.type == SDL_MOUSEWHEEL)
		{
			short zDelta = (short)(event.wheel.y * 120);
			WindowProcedure(m_hWnd, WM_MOUSEWHEEL, MAKEWPARAM(0, zDelta), 0);
		}
		else if (event.type == SDL_KEYDOWN)
		{
			WPARAM vKey = SDLKeycodeToVK(event.key.keysym.sym);
			WindowProcedure(m_hWnd, WM_KEYDOWN, vKey, 0);
		}
		else if (event.type == SDL_KEYUP)
		{
			WPARAM vKey = SDLKeycodeToVK(event.key.keysym.sym);
			WindowProcedure(m_hWnd, WM_KEYUP, vKey, 0);
		}
		else if (event.type == SDL_TEXTINPUT)
		{
			for (size_t i = 0; event.text.text[i] != '\0'; ++i)
			{
				WindowProcedure(m_hWnd, WM_CHAR, (unsigned char)event.text.text[i], 0);
			}
		}
	}

	int mx = 0, my = 0;
	SDL_GetMouseState(&mx, &my);
	Platform_SetCursorPos(mx, my);

	return true;
#else
	MSG msg;

	if (!GetMessage(&msg, NULL, 0, 0))
		return false;

	TranslateMessage(&msg);
	DispatchMessage(&msg);
	return true;
#endif
}

LRESULT CMSApplication::WindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
	}

	return CMSWindow::WindowProcedure(hWnd, uiMsg, wParam, lParam);
}