
#define _CRT_SECURE_NO_WARNINGS


#include "framework.h"
#include "Cliente.h"
#include <Windows.h>
#include <OCIdl.h>
#include <CommCtrl.h>
#include <rdpencomapi.h>
#include <gdiplus.h>
#include <stdio.h>
#include <commdlg.h>
#include <rdpencomapi.h>
#include <string>

#include "associated.h"

#define MAX_LOADSTRING 100


using namespace Gdiplus;
#pragma comment(lib, "gdiplus")
#pragma comment( lib, "comctl32.lib" )

#define APP MAKEINTRESOURCE(101)
#define APPSMALL MAKEINTRESOURCE(102)
#define MAX_ATTENDEE 1
#define override

HWND hwnd, view, ACTIVEX_WINDOW;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static CHAR className[] = "VIEWER";
static HINSTANCE instance = NULL;
IRDPSRAPIViewer* viewerScreen = NULL;

IConnectionPointContainer* p_IntConnPointContainer = NULL;
IConnectionPoint* p_IntConnPoint = NULL;
int dummy = NULL;
bool com_initiated = false;
const char* invitationFile = "C:\\users\\lmarincek\\invitation.xml";   /* Path of the invitation file*/




HINSTANCE hInst;                                // instância atual
WCHAR szTitle[MAX_LOADSTRING];                  // O texto da barra de título
WCHAR szWindowClass[MAX_LOADSTRING];            // o nome da classe da janela principal


LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawWindow(HWND hh);





int ConnectToSharer(IUnknown* Container, REFIID riid, IUnknown* eventHandler, IConnectionPointContainer** p_IntConnPointContainer, IConnectionPoint** p_IntConnPoint)
{
	HRESULT hr = 0;
	unsigned long cookie = 0;
	IConnectionPointContainer* intConnPointContainer = 0;
	IConnectionPoint* intConnPoint = 0;
	*p_IntConnPointContainer = 0;
	*p_IntConnPoint = 0;
	Container->QueryInterface(IID_IConnectionPointContainer, (void**)&intConnPointContainer);
	if (intConnPointContainer)
	{
		*p_IntConnPointContainer = intConnPointContainer;
		intConnPointContainer->FindConnectionPoint(riid, &intConnPoint);
		if (intConnPoint)
		{
			*p_IntConnPoint = intConnPoint;
			hr = intConnPoint->Advise(eventHandler, &cookie);
		}
	}
	return cookie;
}



void DisconnectEvent(IConnectionPointContainer* intConnPointContainer, IConnectionPoint* intConnPoint, unsigned int Cookie)
{
	unsigned long hr = 0;
	OutputDebugString("DisconnectEvent");
	intConnPoint->Unadvise(Cookie);
	intConnPoint->Release();
	intConnPointContainer->Release();
}

void FailedToConnect();

class EventHandling : public _IRDPSessionEvents {
public:
	EventHandling() {
	}
	~EventHandling() {
	}

	virtual HRESULT STDMETHODCALLTYPE override QueryInterface(
		REFIID iid, void** ppvObject) {
		*ppvObject = 0;
		if (iid == IID_IUnknown || iid == IID_IDispatch || iid == __uuidof(_IRDPSessionEvents))
			*ppvObject = this;
		if (*ppvObject)
		{
			((IUnknown*)(*ppvObject))->AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE override AddRef(void) {
		return 0;
	}

	virtual ULONG STDMETHODCALLTYPE override Release(void) {
		return 0;
	}


	/* Override IDispatch virtual methods */
	virtual HRESULT STDMETHODCALLTYPE override GetTypeInfoCount(
		__RPC__out UINT* pctinfo) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE override GetTypeInfo(
		UINT iTInfo,
		LCID lcid,
		__RPC__deref_out_opt ITypeInfo** ppTInfo) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE override GetIDsOfNames(
		__RPC__in REFIID riid,
		__RPC__in_ecount_full(cNames) LPOLESTR* rgszNames,
		UINT cNames,
		LCID lcid,
		__RPC__out_ecount_full(cNames) DISPID* rgDispId) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE override Invoke(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS FAR* pDispParams,
		VARIANT FAR* pVarResult,
		EXCEPINFO FAR* pExcepInfo,
		unsigned int FAR* puArgErr) {


		switch (dispIdMember) {
		case DISPID_RDPSRAPI_EVENT_ON_VIEWER_CONNECTFAILED:
			FailedToConnect();
			break;
		case DISPID_RDPSRAPI_EVENT_ON_VIEWER_CONNECTED:
			break;

		case DISPID_RDPSRAPI_EVENT_ON_ERROR:
			break;
		}
		return S_OK;
	}
};

EventHandling eventHandling;

void GDIPLUS(HDC hdc) {
	Graphics graphics(hdc);
	const wchar_t* text = L"RDP Client";

	FontFamily family(L"Courier");
	Font font(&family, 20, FontStyleRegular, UnitPixel);


	SolidBrush sbrush(Color::Black);
	graphics.DrawString(text, wcslen(text), &font, PointF(20, 200), &sbrush);
}


void InitializeCOM() {
	if (!com_initiated)
	{
	HRESULT l_result=	CoInitialize(0);
		com_initiated = true;
	}
}
void UninitializeCOM() {
	if (com_initiated)
	{
		CoUninitialize();
		com_initiated = false;
	}
}


bool ConnectToSharer() {

	bool returnValue = true;
	if (viewerScreen == NULL) {
				HRESULT hr = CoCreateInstance(__uuidof(RDPViewer),
					NULL, CLSCTX_INPROC_SERVER,
					IID_PPV_ARGS(&viewerScreen));

				if (SUCCEEDED(hr)) {

					dummy = ConnectToSharer(reinterpret_cast<IUnknown*>(viewerScreen), __uuidof(_IRDPSessionEvents), reinterpret_cast<IUnknown*>( & eventHandling), &p_IntConnPointContainer, &p_IntConnPoint);

					FILE* fileHandle = NULL;
					fileHandle=fopen(invitationFile, "r");
					if (fileHandle) {
						WCHAR inviteString[4096];
						ZeroMemory(inviteString, sizeof(inviteString));
						fgetws(inviteString, 4096, fileHandle);
						fclose(fileHandle);
						BSTR b_inviteString = SysAllocString(inviteString);
						BSTR b_name = SysAllocString(L"");
						const BSTR b_password = SysAllocString(L"");
						if (viewerScreen->Connect(b_inviteString, b_name, b_password) == S_OK) {
							returnValue = true;
						}
						else {
							returnValue = false;
						}

//						SysFreeString(b_inviteString);
//						SysFreeString(b_name);
//						SysFreeString(b_password);

					}
					else {
						returnValue = false;
					}
				}
				else {
					viewerScreen = NULL;
					returnValue  = false;
				}
			}
			else {
				returnValue = false;
			}

			return returnValue;
}

void DisconnectFromSharer() {
	if (viewerScreen != NULL) {
		DisconnectEvent(p_IntConnPointContainer, p_IntConnPoint, dummy);
		viewerScreen->Disconnect();
		viewerScreen->Release();
		viewerScreen = NULL;
	}

}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Coloque o código aqui.
	WNDCLASSEX WndClass;
	instance = hInstance;

	AXRegister();
	INITCOMMONCONTROLSEX icex = { 0 };
	InitCommonControlsEx(&icex);

	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = instance;
	WndClass.hIcon = LoadIcon(hInstance, APP);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = CreateSolidBrush(RGB(245, 247, 248));
	WndClass.lpszMenuName = 0;
	WndClass.lpszClassName = className;
	WndClass.hIconSm = LoadIcon(hInstance, APPSMALL);

	RegisterClassEx(&WndClass);

	hwnd = CreateWindowEx(
		0,
		className,
		"RDP Viewer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
        1900,1000,
		NULL, NULL,
		instance,
		NULL);

	RECT wInfo;
	GetClientRect(hwnd, &wInfo);

	int Width = wInfo.right;
	int Height = wInfo.bottom;


	view = CreateWindow("edit", 0, WS_CLIPCHILDREN | WS_CHILD | WS_VISIBLE, 5, 5, Width - 10, Height - 10, hwnd, 0, instance, 0);



	ShowWindow(hwnd, 1);
	UpdateWindow(hwnd);

	SendMessage(view, EM_SETREADONLY, 1, 0);
	SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);

	// Inicializar cadeias de caracteres globais
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CLIENTE, szWindowClass, MAX_LOADSTRING);

	InitializeCOM();


	if (ConnectToSharer()) {
		DrawWindow(view);
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENTE));

	MSG msg;

	// Loop de mensagem principal:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		std::string stringOutput;

		OutputDebugStringA(stringOutput.c_str());
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			OutputDebugStringA(stringOutput.c_str());

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}






void DrawWindow(HWND hh) {
	// While looking at the calls made I have never seen this create window called it is always returns 0 and is recreated 
	ACTIVEX_WINDOW = CreateWindowEx(0, "AX", "{32be5ed2-5c86-480f-a914-0ff8885a1b3f}", WS_CHILD | WS_VISIBLE, 0, 20, 1, 1, hh, 0, instance, 0);
	if (ACTIVEX_WINDOW) {
		IUnknown* a = 0;
		SendMessage(ACTIVEX_WINDOW, AX_QUERYINTERFACE, (WPARAM) & __uuidof(IUnknown*), (LPARAM)&a);
	}
	else {
		ACTIVEX_WINDOW = CreateWindow("AX", "}32BE5ED2-5C86-480F-A914-0FF8885A1B3F}", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, 0, 1, 1, hh, 0, instance, 0);
		SendMessage(ACTIVEX_WINDOW, AX_RECREATE, 0, (LPARAM)viewerScreen);
	}

	SendMessage(ACTIVEX_WINDOW, AX_INPLACE, 1, 0);
	ShowWindow(ACTIVEX_WINDOW, SW_MAXIMIZE);
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	std::string stringOutput;
	stringOutput = "Executanto WndProc com message=" + std::to_string(Message) + "\n";


	switch (Message) {

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		GDIPLUS(hdc);
		EndPaint(hwnd, &ps);
		break;

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pDIS;
		pDIS = (LPDRAWITEMSTRUCT)lParam;
		CHAR staticText[99];
		LRESULT len = SendMessage(pDIS->hwndItem, WM_GETTEXT, ARRAYSIZE(staticText), (LPARAM)staticText);
	}
	break;
	case WM_COMMAND:
		break;

	case WM_CLOSE:
		DisconnectFromSharer();
		UninitializeCOM();
		DestroyWindow(view);
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

void FailedToConnect() {
	DisconnectFromSharer();
}

