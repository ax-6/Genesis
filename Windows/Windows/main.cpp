#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>

#include "WebView2.h"

using namespace Microsoft::WRL;

// Global variables

// The main window class name.
static TCHAR szWindowClass[] = _T("Project Genesis");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Project Genesis");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewController;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webview;

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		return 1;
	}

	// Store instance handle in our global variable
	hInst = hInstance;

	// The parameters to CreateWindow explained:
	// szWindowClass: the name of the application
	// szTitle: the text that appears in the title bar
	// WS_OVERLAPPEDWINDOW: the type of window to create
	// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
	// 500, 100: initial size (width, length)
	// NULL: the parent of this window
	// NULL: this application does not have a menu bar
	// hInstance: the first parameter from WinMain
	// NULL: not used in this application
	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1200, 900,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		return 1;
	}

	// The parameters to ShowWindow explained:
	// hWnd: the value returned from CreateWindow
	// nCmdShow: the fourth parameter from WinMain
	ShowWindow(hWnd,
		nCmdShow);
	UpdateWindow(hWnd);

	// <-- WebView2 sample code starts here -->
	// Step 3 - Create a single WebView within the parent window
	// Locate the browser and set up the environment for WebView
	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                env->CreateCoreWebView2Controller(hWnd, 
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (controller) {
                                webviewController = controller;
                                webviewController->get_CoreWebView2(&webview);

                                // 配置WebView2设置
                                wil::com_ptr<ICoreWebView2Settings> settings;
                                webview->get_Settings(&settings);
                                settings->put_IsWebMessageEnabled(TRUE);

                                // 设置初始大小
                                RECT bounds;
                                GetClientRect(hWnd, &bounds);
                                webviewController->put_Bounds(bounds);

                                // 加载本地HTML界面
                                webview->Navigate(L"file:///F:/Genesis/Windows/Windows/main.html");

                                // 注册消息处理
                                EventRegistrationToken token;
                                webview->add_WebMessageReceived(
                                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                        [](ICoreWebView2* webview, 
                                           ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                            wil::unique_cotaskmem_string message;
                                            args->TryGetWebMessageAsString(&message);
                                            
                                            // 解析导航指令
                                            if (wcscmp(message.get(), L"nav:back") == 0) {
                                                webview->GoBack();
                                            }
                                            else if (wcscmp(message.get(), L"nav:forward") == 0) {
                                                webview->GoForward();
                                            }
                                            else if (wcsstr(message.get(), L"nav:to:") != nullptr) {
                                                const wchar_t* url = message.get() + 7;
                                                webview->Navigate(url);
                                            }
                                            return S_OK;
                                        }).Get(), &token);

                                // 监听导航完成事件
                                webview->add_NavigationCompleted(
                                    Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                        [](ICoreWebView2* webview, 
                                           ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                            BOOL success;
                                            args->get_IsSuccess(&success);
                                            if (success) {
                                                wil::unique_cotaskmem_string uri;
                                                webview->get_Source(&uri);
                                                
                                                // 发送当前URL回HTML界面
                                                std::wstring script = L"updateAddressBar('";
                                                script += uri.get();
                                                script += L"');";
                                                webview->ExecuteScript(script.c_str(), nullptr);
                                            }
                                            return S_OK;
                                        }).Get(), &token);
                            }
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());



	// <-- WebView2 sample code ends here -->

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR greeting[] = _T("Hello, Windows desktop!");

	switch (message)
	{
	case WM_SIZE:
		if (webviewController != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			webviewController->put_Bounds(bounds);
		};
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}