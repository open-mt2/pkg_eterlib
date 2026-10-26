#include "StdAfx.h"
#include "Input.h"

#if defined(__LINUX__)
#include <SDL2/SDL.h>
#endif

LPDIRECTINPUT8			CInputDevice::ms_lpDI = NULL;
LPDIRECTINPUTDEVICE8	CInputKeyboard::ms_lpKeyboard = NULL;
bool					CInputKeyboard::ms_bPressedKey[256];
char					CInputKeyboard::ms_diks[256];

CInputDevice::CInputDevice()
{
}

CInputDevice::~CInputDevice()
{
	SAFE_RELEASE(ms_lpDI);
}

HRESULT CInputDevice::CreateDevice(HWND /*hWnd*/)
{
	if (ms_lpDI)
	{
		ms_lpDI->AddRef();
		return S_OK;
	}

	HRESULT hr;
	
	// Create a DInput object
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
					IID_IDirectInput8, (VOID**) &ms_lpDI, NULL)))
		return hr;

	return S_OK;
}


CInputKeyboard::CInputKeyboard()
{
	ResetKeyboard();
}

CInputKeyboard::~CInputKeyboard()
{
	if (ms_lpKeyboard)
		ms_lpKeyboard->Unacquire();

	SAFE_RELEASE(ms_lpKeyboard);
}

void CInputKeyboard::ResetKeyboard()
{
	memset(ms_diks, 0, sizeof(ms_diks));
	memset(ms_bPressedKey, 0, sizeof(ms_bPressedKey));
}

bool CInputKeyboard::InitializeKeyboard(HWND hWnd)
{
	if (ms_lpKeyboard)
		return true;

	if (FAILED(CreateDevice(hWnd)))
		return false;
	
	HRESULT hr;

	// Obtain an interface to the system keyboard device.
	if (FAILED(hr = ms_lpDI->CreateDevice(GUID_SysKeyboard, &ms_lpKeyboard, NULL)))
		return false;

	if (FAILED(hr = ms_lpKeyboard->SetDataFormat(&c_dfDIKeyboard)))
		return false;

// Alt + F4를 위해 비독점 모드로 - [levites]
//	DWORD dwCoopFlags = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
//	DWORD dwCoopFlags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
	DWORD dwCoopFlags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

	if (FAILED(hr = ms_lpKeyboard->SetCooperativeLevel(hWnd, dwCoopFlags)))
		return false;

	ms_lpKeyboard->Acquire();

	return true;
}

void CInputKeyboard::UpdateKeyboard()
{
#if defined(__LINUX__)
	const Uint8* state = SDL_GetKeyboardState(NULL);
	if (state)
	{
		auto checkKey = [&](SDL_Scancode sc, int dik) {
			if (state[sc]) ms_diks[dik] = (char)0x80;
			else ms_diks[dik] = 0;
		};

		checkKey(SDL_SCANCODE_W, DIK_W);
		checkKey(SDL_SCANCODE_A, DIK_A);
		checkKey(SDL_SCANCODE_S, DIK_S);
		checkKey(SDL_SCANCODE_D, DIK_D);
		checkKey(SDL_SCANCODE_Q, DIK_Q);
		checkKey(SDL_SCANCODE_E, DIK_E);
		checkKey(SDL_SCANCODE_R, DIK_R);
		checkKey(SDL_SCANCODE_F, DIK_F);
		checkKey(SDL_SCANCODE_SPACE, DIK_SPACE);
		checkKey(SDL_SCANCODE_ESCAPE, DIK_ESCAPE);
		checkKey(SDL_SCANCODE_RETURN, DIK_RETURN);
		checkKey(SDL_SCANCODE_KP_ENTER, DIK_NUMPADENTER);
		checkKey(SDL_SCANCODE_LSHIFT, DIK_LSHIFT);
		checkKey(SDL_SCANCODE_RSHIFT, DIK_RSHIFT);
		checkKey(SDL_SCANCODE_LCTRL, DIK_LCONTROL);
		checkKey(SDL_SCANCODE_RCTRL, DIK_RCONTROL);
		checkKey(SDL_SCANCODE_LALT, DIK_LMENU);
		checkKey(SDL_SCANCODE_RALT, DIK_RMENU);
		checkKey(SDL_SCANCODE_TAB, DIK_TAB);
		checkKey(SDL_SCANCODE_BACKSPACE, DIK_BACK);
		checkKey(SDL_SCANCODE_INSERT, DIK_INSERT);
		checkKey(SDL_SCANCODE_DELETE, DIK_DELETE);
		checkKey(SDL_SCANCODE_HOME, DIK_HOME);
		checkKey(SDL_SCANCODE_END, DIK_END);
		checkKey(SDL_SCANCODE_PAGEUP, DIK_PRIOR);
		checkKey(SDL_SCANCODE_PAGEDOWN, DIK_NEXT);
		checkKey(SDL_SCANCODE_UP, DIK_UP);
		checkKey(SDL_SCANCODE_DOWN, DIK_DOWN);
		checkKey(SDL_SCANCODE_LEFT, DIK_LEFT);
		checkKey(SDL_SCANCODE_RIGHT, DIK_RIGHT);
		checkKey(SDL_SCANCODE_1, DIK_1);
		checkKey(SDL_SCANCODE_2, DIK_2);
		checkKey(SDL_SCANCODE_3, DIK_3);
		checkKey(SDL_SCANCODE_4, DIK_4);
		checkKey(SDL_SCANCODE_5, DIK_5);
		checkKey(SDL_SCANCODE_6, DIK_6);
		checkKey(SDL_SCANCODE_7, DIK_7);
		checkKey(SDL_SCANCODE_8, DIK_8);
		checkKey(SDL_SCANCODE_9, DIK_9);
		checkKey(SDL_SCANCODE_0, DIK_0);
		checkKey(SDL_SCANCODE_F1, DIK_F1);
		checkKey(SDL_SCANCODE_F2, DIK_F2);
		checkKey(SDL_SCANCODE_F3, DIK_F3);
		checkKey(SDL_SCANCODE_F4, DIK_F4);
	}
#else
	if (!ms_lpKeyboard)
		return;

	HRESULT hr;
	
	hr = ms_lpKeyboard->GetDeviceState(sizeof(ms_diks), &ms_diks);

	if (FAILED(hr))
	{
		hr = ms_lpKeyboard->Acquire();			
		
		// 현재 어플리케이션이 비활성화 되어 있어 입력을 받을 수 없다.
		//if (hr == DIERR_OTHERAPPHASPRIO || hr == DIERR_NOTACQUIRED);
		return;
	}
#endif

	for (int i = 0; i < 256; ++i)
	{
		if (ms_diks[i] & 0x80)
		{
			if (!IsPressed(i))
				KeyDown(i);
		}
		else if (IsPressed(i))
			KeyUp(i);
	}
}

void CInputKeyboard::KeyDown(int iIndex)
{
	ms_bPressedKey[iIndex] = true;
	OnKeyDown(iIndex);
}

void CInputKeyboard::KeyUp(int iIndex)
{
	ms_bPressedKey[iIndex] = false;
	OnKeyUp(iIndex);
}

bool CInputKeyboard::IsPressed(int iIndex)
{
	return ms_bPressedKey[iIndex];
}
