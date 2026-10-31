// SDL-backed IME (T1.6).
//
// The Windows IMM/TSF implementation in IME.cpp is compiled out when
// OPENMT2_SDL_WINDOW is defined (both targets). SDL delivers the platform IME
// as SDL_TEXTEDITING (composition) and SDL_TEXTINPUT (committed text); this
// file turns those events into the text buffer the Python `ime` module reads.
//
// Limitation: SDL does not expose the candidate list or the reading window, so
// those APIs return empty results. The OS IME window is still shown by SDL on
// platforms that provide one (Windows, IBus/Fcitx on Linux).

#include "StdAfx.h"
#include "IME.h"
#include <EterBase/Utils.h>

#if defined(OPENMT2_SDL_WINDOW)

#include <SDL2/SDL.h>

bool CIME::ms_bInitialized = false;
bool CIME::ms_bDisableIMECompletely = false;
bool CIME::ms_bUILessMode = false;
bool CIME::ms_bImeEnabled = false;
bool CIME::ms_bCaptureInput = false;
bool CIME::ms_bChineseIME = false;
bool CIME::ms_bUseIMMCandidate = false;

HWND CIME::ms_hWnd = nullptr;
HKL CIME::ms_hklCurrent = nullptr;
char CIME::ms_szKeyboardLayout[KL_NAMELENGTH + 1] = {};
OSVERSIONINFOA CIME::ms_stOSVI = {};

HINSTANCE CIME::ms_hImm32Dll = nullptr;
HINSTANCE CIME::ms_hCurrentImeDll = nullptr;
DWORD CIME::ms_dwImeState = 0;
DWORD CIME::ms_adwId[2] = {0, 0};

DWORD CIME::ms_dwIMELevel = 3;
DWORD CIME::ms_dwIMELevelSaved = 3;

bool CIME::ms_bCandidateList = false;
DWORD CIME::ms_dwCandidateCount = 0;
bool CIME::ms_bVerticalCandidate = false;
int CIME::ms_iCandListIndexBase = 0;
WCHAR CIME::ms_wszCandidate[CIME::MAX_CANDLIST][CIME::MAX_CANDIDATE_LENGTH] = {};
DWORD CIME::ms_dwCandidateSelection = 0;
DWORD CIME::ms_dwCandidatePageSize = 0;

bool CIME::ms_bReadingInformation = false;
int CIME::ms_iReadingError = 0;
bool CIME::ms_bHorizontalReading = false;
std::vector<wchar_t> CIME::ms_wstrReading;

wchar_t* CIME::ms_wszCurrentIndicator = nullptr;

IIMEEventSink* CIME::ms_pEvent = nullptr;

wchar_t CIME::m_wText[IMESTR_MAXLEN] = {};
int CIME::ms_compLen = 0;
int CIME::ms_curpos = 0;
int CIME::ms_lastpos = 0;
int CIME::ms_ulbegin = 0;
int CIME::ms_ulend = 0;

UINT CIME::ms_uOutputCodePage = 65001;
UINT CIME::ms_uInputCodePage = 65001;

std::string CIME::m_stText;
std::string CIME::m_stComposition;

CIME::CIME()
	: m_hOrgIMC(nullptr)
	, m_max(IMESTR_MAXLEN)
	, m_userMax(IMESTR_MAXLEN)
	, m_bOnlyNumberMode(false)
	, m_bEnablePaste(true)
	, m_bUseDefaultIME(false)
{
}

CIME::~CIME() = default;

bool CIME::Initialize(HWND hWnd)
{
	ms_hWnd = hWnd;
	ms_bInitialized = true;
	ms_bImeEnabled = true;
	ms_dwImeState = 1;
	Clear();
	SDL_StartTextInput();
	return true;
}

void CIME::Uninitialize()
{
	SDL_StopTextInput();
	ms_bImeEnabled = false;
	ms_bInitialized = false;
	ms_hWnd = nullptr;
}

void CIME::Clear()
{
	memset(m_wText, 0, sizeof(m_wText));
	ms_compLen = 0;
	ms_curpos = 0;
	ms_lastpos = 0;
	ms_ulbegin = 0;
	ms_ulend = 0;
	m_stText.clear();
	m_stComposition.clear();
}

void CIME::SetMax(int iMax) { m_max = iMax; }
void CIME::SetUserMax(int iMax) { m_userMax = iMax; }

void CIME::SetText(const char* c_szText, int len)
{
	if (!c_szText)
		return;
	m_stText.assign(c_szText, len < 0 ? strlen(c_szText) : (size_t)len);
	ms_curpos = (int)m_stText.size();
	ms_lastpos = ms_curpos;
}

int CIME::GetText(std::string& rstrText, bool /*addCodePage*/)
{
	rstrText.append(m_stText);
	return (int)m_stText.size();
}

const char* CIME::GetCodePageText() { return ""; }
int CIME::GetCodePage() { return (int)ms_uOutputCodePage; }

int CIME::GetCandidateCount() { return 0; }
int CIME::GetCandidatePageCount() { return 0; }
int CIME::GetCandidate(DWORD, std::string&) { return 0; }
int CIME::GetCandidateSelection() { return 0; }
int CIME::GetReading(std::string&) { return 0; }
int CIME::GetReadingError() { return 0; }

void CIME::SetInputMode(DWORD dwMode) { ms_dwImeState = dwMode; }
DWORD CIME::GetInputMode() { return ms_dwImeState; }

bool CIME::IsIMEEnabled() { return ms_bImeEnabled; }

void CIME::EnableIME(bool bEnable)
{
	ms_bImeEnabled = bEnable;
	if (bEnable) SDL_StartTextInput();
	else SDL_StopTextInput();
}

void CIME::DisableIME() { EnableIME(false); }

void CIME::EnableCaptureInput() { ms_bCaptureInput = true; }
void CIME::DisableCaptureInput() { ms_bCaptureInput = false; }
bool CIME::IsCaptureEnabled() { return ms_bCaptureInput; }

void CIME::SetNumberMode() { m_bOnlyNumberMode = true; }
void CIME::SetStringMode() { m_bOnlyNumberMode = false; }

bool CIME::__IsWritable(wchar_t key)
{
	for (wchar_t except : m_exceptKey)
		if (except == key)
			return false;
	return true;
}

void CIME::AddExceptKey(wchar_t key) { m_exceptKey.push_back(key); }
void CIME::ClearExceptKey() { m_exceptKey.clear(); }

void CIME::PasteTextFromClipBoard()
{
	char* clipboard = SDL_GetClipboardText();
	if (clipboard)
	{
		PasteString(clipboard);
		SDL_free(clipboard);
	}
}

void CIME::EnablePaste(bool bFlag) { m_bEnablePaste = bFlag; }

void CIME::PasteString(const char* str)
{
	if (!m_bEnablePaste || !str)
		return;
	m_stText.append(str);
	ms_curpos = ms_lastpos = (int)m_stText.size();
}

void CIME::FinalizeString(bool /*bSend*/) { ms_compLen = 0; }

void CIME::UseDefaultIME() { m_bUseDefaultIME = true; }

int CIME::GetCurPos() { return ms_curpos; }
int CIME::GetCompLen() { return ms_compLen; }
int CIME::GetULBegin() { return ms_ulbegin; }
int CIME::GetULEnd() { return ms_ulend; }

void CIME::CloseCandidateList() { ms_bCandidateList = false; ms_dwCandidateCount = 0; }
void CIME::CloseReadingInformation() { ms_bReadingInformation = false; }
void CIME::ChangeInputLanguage() {}
void CIME::ChangeInputLanguageWorker() {}

LRESULT CIME::WMInputLanguage(HWND, UINT, WPARAM, LPARAM) { return 0; }
LRESULT CIME::WMStartComposition(HWND, UINT, WPARAM, LPARAM) { return 0; }
LRESULT CIME::WMComposition(HWND, UINT, WPARAM, LPARAM) { return 0; }
LRESULT CIME::WMEndComposition(HWND, UINT, WPARAM, LPARAM) { return 0; }
LRESULT CIME::WMNotify(HWND, UINT, WPARAM, LPARAM) { return 0; }
LRESULT CIME::WMChar(HWND, UINT, WPARAM, LPARAM) { return 0; }

void CIME::IncCurPos() { if (ms_curpos < ms_lastpos) ++ms_curpos; }
void CIME::DecCurPos() { if (ms_curpos > 0) --ms_curpos; }
void CIME::SetCurPos(int offset) { ms_curpos = offset < 0 ? 0 : (offset > ms_lastpos ? ms_lastpos : offset); }
void CIME::DelCurPos()
{
	// Delete key semantics: remove the character at the cursor, not before it.
	if (ms_curpos < ms_lastpos)
	{
		m_stText.erase(ms_curpos, 1);
		--ms_lastpos;
	}
}

void CIME::CheckInputLocale() {}
void CIME::CheckToggleState() {}
void CIME::SetSupportLevel(DWORD dwImeLevel) { ms_dwIMELevel = dwImeLevel; }

void CIME::InsertString(wchar_t* /*szString*/, int /*iSize*/) {}
void CIME::OnChar(wchar_t c)
{
	if (m_bOnlyNumberMode && !iswdigit(c))
		return;
	if (!__IsWritable(c))
		return;
	m_stText.push_back((char)c);
	ms_curpos = ms_lastpos = (int)m_stText.size();
}

UINT CIME::GetCodePageFromLang(LANGID) { return 65001; }
void CIME::ResultProcess(HIMC) {}
void CIME::CompositionProcessBuilding(HIMC) {}
void CIME::CompositionProcess(HIMC) {}
void CIME::AttributeProcess(HIMC) {}
void CIME::CandidateProcess(HIMC) {}
void CIME::ReadingProcess(HIMC) {}
bool CIME::IsMax(const wchar_t*, int) { return false; }
DWORD CIME::GetImeId(UINT) { return 0; }
bool CIME::GetReadingWindowOrientation() { return false; }
void CIME::SetupImeApi() {}

void CIME::OnSDLTextEditing(const char* utf8, int start, int length)
{
	m_stComposition = utf8 ? utf8 : "";
	ms_compLen = (int)m_stComposition.size();
	ms_ulbegin = start;
	ms_ulend = start + length;
	if (ms_pEvent)
		ms_pEvent->OnUpdate();
}

void CIME::OnSDLTextInput(const char* utf8)
{
	if (!utf8)
		return;
	m_stComposition.clear();
	ms_compLen = 0;
	m_stText.append(utf8);
	ms_curpos = ms_lastpos = (int)m_stText.size();
	if (ms_pEvent)
		ms_pEvent->OnUpdate();
}

#endif // OPENMT2_SDL_WINDOW
