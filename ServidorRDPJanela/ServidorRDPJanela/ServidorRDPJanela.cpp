#define teste 1
#define teste_audio 1



#include "framework.h"
#include "WindowsProject1.h"
#include <Windows.h>
#include <rdpencomapi.h>
#include <gdiplus.h>
#include <commdlg.h>
#include <stdio.h>
#include <string>






#define _CRT_SECURE_NO_WARNINGS


using namespace Gdiplus;

#pragma comment(lib, "gdiplus")

#define APP MAKEINTRESOURCE(101)
#define APPSMALL MAKEINTRESOURCE(102)
#define MAX_VIEWERS 5
#define override


HWND hwnd;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static CHAR className[] = "SHARER";
static HINSTANCE instance = NULL;

IRDPSRAPISharingSession* share_session = NULL;
IRDPSRAPIInvitationManager* invitationManager = NULL;
IRDPSRAPIInvitation* invitation = NULL;
IRDPSRAPIAttendeeManager* viewersManager = NULL;
IRDPSRAPIAudioStream* audioStream = NULL;

IConnectionPointContainer* conn_point_container = NULL;
IConnectionPoint* conn_point = NULL;



/* Event Handlers*/
void ViewerConnected(IDispatch* pAttendee);
void ViewerDisconnected(IDispatch* pAttendee);


/* Override virtual methods of _IRDPSessionEvents */
class EventHandling : public _IRDPSessionEvents 
{
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

       if (dispIdMember == DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_CONNECTED)
       {
           ViewerConnected(pDispParams->rgvarg[0].pdispVal);
       }
       else if (dispIdMember == DISPID_RDPSRAPI_EVENT_ON_ATTENDEE_CONNECTED)
       {
           ViewerDisconnected(pDispParams->rgvarg[0].pdispVal);
       }
       return S_OK;
    }
};

EventHandling eventHandling;





void GDIPLUS(HDC hdc, int drvIndex = -1) {
    Graphics graphics(hdc);
    const wchar_t* text = L"Screen Sharing";

    FontFamily family(L"Courier");
    Font font(&family, 15, FontStyleRegular, UnitPixel);

    SolidBrush bBrush(Color(255, 0, 100, 200));
    graphics.FillRectangle(&bBrush, 0, 0, 800, 85);

    SolidBrush sbrush(Color::Black);
    graphics.DrawString(text, wcslen(text), &font, PointF(20, 100), &sbrush);
}


void COM_INIT() {
    HRESULT l_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
   }

void COM_UNIN() {
    CoUninitialize();
}


bool GenerateInvitation()
{
	bool invitation_ok = false;
	BSTR b_authString = SysAllocString(L"Sharer");
	BSTR b_groupName = SysAllocString(L"");
	BSTR b_password = SysAllocString(L"");
	if (invitationManager->CreateInvitation(
		b_authString,
		b_groupName,
		b_password,
		MAX_VIEWERS,
		&invitation) == S_OK)
	{
		const char* fileName = "C:\\Users\\lmarincek\\Documents\\invitation.xml";  /* fill with the path of the invitation file to be created */

		FILE* invitationFile = NULL;
		fopen_s(&invitationFile, fileName, "w");
		if (invitationFile) {
			BSTR inviteString;
			if (invitation->get_ConnectionString(&inviteString) == S_OK)
			{
				fprintf_s(invitationFile, "%ws", inviteString);
				SysFreeString(inviteString);
			}

			fclose(invitationFile);
			invitation_ok = true;
		}

		if (share_session->get_Attendees(&viewersManager) == S_OK)
		{
            OutputDebugString("viewersManager OK\n");
		}
	}
	else
	{
		/* Invitation NOK*/
	}
	SysFreeString(b_authString);
	SysFreeString(b_groupName);
	SysFreeString(b_password);

	return invitation_ok;
}

void share_screen() {
	if (share_session == NULL) {

		HRESULT hr = CoCreateInstance(__uuidof(RDPSession),
			NULL, CLSCTX_ALL,
			__uuidof(IRDPSRAPISharingSession),
			reinterpret_cast<void**>(&share_session));

		if (SUCCEEDED(hr)) {
			IUnknown* unknown_ptr = reinterpret_cast<IUnknown*>(share_session);
			IUnknown* eventWatcher = reinterpret_cast<IUnknown*>(&eventHandling);
			IConnectionPointContainer** l_conn_point_container = &conn_point_container;
			IConnectionPoint** l_conn_point = &conn_point;
			unsigned long cookie = 0;
			IConnectionPointContainer* conn_point_container = 0;
			IConnectionPoint* conn_point = 0;
			*l_conn_point_container = 0;
			*l_conn_point = 0;

			hr = unknown_ptr->QueryInterface(IID_IConnectionPointContainer, (void**)&conn_point_container);
			if (SUCCEEDED(hr))
			{
				*l_conn_point_container = conn_point_container;
				hr = conn_point_container->FindConnectionPoint(__uuidof(_IRDPSessionEvents), &conn_point);
				if (SUCCEEDED(hr))
				{
					*l_conn_point = conn_point;
					hr = conn_point->Advise(eventWatcher, &cookie);
				}
			}

			if (share_session->Open() == S_OK) {
				if (share_session->get_Invitations(&invitationManager) == S_OK) {
					if (GenerateInvitation())
					{
                        if (audioStream == NULL)
                        {
                            /* Is this correct??? */
                            hr = share_session->QueryInterface(__uuidof(IRDPSRAPIAudioStream), (void**)&audioStream);
                            if (SUCCEEDED(hr))
                            {
                                OutputDebugString("audioStream OK\n");
                                audioStream->AddRef();
                            }
                            else
                            {
                                OutputDebugString("Failed to obtain audioStream from Session\n");
                                audioStream = NULL;
                            }
                        }
					}
				}
			}
		}
	}
}


void ViewerConnected(IDispatch* pViewer) {
    IRDPSRAPIAttendee* pRDPViewer;
    pViewer->QueryInterface(__uuidof(IRDPSRAPIAttendee), (void**)&pRDPViewer);
    pRDPViewer->put_ControlLevel(CTRL_LEVEL::CTRL_LEVEL_VIEW);
    std::string outputString;
    HRESULT hr;
    if (audioStream == NULL)
    {

        /* Is this correct???? */
        hr = pViewer->QueryInterface(__uuidof(IRDPSRAPIAudioStream), (void**)&audioStream);
        if (SUCCEEDED(hr))
        {
            OutputDebugString("Got audioStream from pViewer\n");
            __int64 audioPeriod;
            bool audioOk = false;
            audioStream->AddRef();
            hr = audioStream->Initialize(&audioPeriod);
            if (SUCCEEDED(hr))
            {
                outputString = "audioStream started with period=" + std::to_string(audioPeriod) + "\n";
                OutputDebugString(outputString.c_str());
                hr = audioStream->Start();
                if (SUCCEEDED(hr))
                {
                    OutputDebugString("audioStream started OK\n");
                    audioOk = true;
                }
                else
                {
                    OutputDebugString("Failed to start audioStream\n");
                }
            }
            else
            {
                OutputDebugString("Failed to initialize audioStream\n");
            }
            if (!audioOk)
            {
                audioStream->Release();
                audioStream = NULL;
            }
        }
        else
        {
            outputString = "Failed to get audioStream from Viewer: " + std::to_string(hr) + "\n";
            OutputDebugString(outputString.c_str());
            audioStream = NULL;
        }

    }
 }



void ViewerDisconnected(IDispatch* viewer) {
    viewer->Release();
    conn_point = 0;
    conn_point_container = 0;
}



int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    WNDCLASSEX WndClass;
    MSG Msg;
    instance = hInstance;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


    WndClass.cbSize = sizeof(WNDCLASSEX);
    WndClass.style = NULL;
    WndClass.lpfnWndProc = WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = instance;
    WndClass.hIcon = LoadIcon(hInstance, APP);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
    WndClass.lpszMenuName = 0;
    WndClass.lpszClassName = className;
    WndClass.hIconSm = LoadIcon(hInstance, APPSMALL);



    RegisterClassEx(&WndClass);
    hwnd = CreateWindowEx(
        0,
        className,
        "DESKTOP SHARER",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        100, 100,
        NULL, NULL,
        instance,
        NULL);
    
    COM_INIT();

    ShowWindow(hwnd, 1);
    UpdateWindow(hwnd);

    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);

    share_screen();

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));
    BOOL bRet;

    while ((bRet=GetMessage(&Msg, NULL, 0, 0))!= 0) {
        MSG msgRec = Msg;
        UINT ultimaMensagem = Msg.message;
        if (!TranslateAccelerator(Msg.hwnd, hAccelTable, &Msg))
        if (bRet != -1)
        {
           if (IsWindow(Msg.hwnd))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    return Msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    switch (Message) {
    case WM_PAINT:
    {
        hdc = BeginPaint(hwnd, &ps);
        GDIPLUS(hdc);
        EndPaint(hwnd, &ps);
    }
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
        {
            COM_UNIN();
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        {
            PostQuitMessage(0);

        }
       break;

    
    default:
        return DefWindowProc(hwnd, Message, wParam, lParam);
    }
    return 0;
}












