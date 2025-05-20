// Copyright 2025 AQ author, All Rights Reserved.
// This program is licensed under the AQ License. You can find the AQ license in
// the root directory.

#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>

#include "WebView2.h"

// #include "main.h"

// Global variables

// The main window class name.
static TCHAR szWindowClass[] = _T("Project Genesis");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Project Genesis");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> page_webviewController;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> page_webview;

HINSTANCE global_hInstance;
HINSTANCE global_hPrevInstance;
LPSTR     global_lpCmdLine;
int       global_nCmdShow;

extern "C" {
	int CALLBACK WindowsStart(const char* url)
	{
		HINSTANCE hInstance = global_hInstance;
		HINSTANCE hPrevInstance = global_hPrevInstance;
			LPSTR     lpCmdLine = global_lpCmdLine;
		int       nCmdShow = global_nCmdShow;

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
			Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
				[hWnd, url](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

					// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
					env->CreateCoreWebView2Controller(hWnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
						[hWnd, url](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
							if (controller != nullptr) {
								page_webviewController = controller;
								page_webviewController->get_CoreWebView2(&page_webview);
							}

							// Add a few settings for the webview
							// The demo step is redundant since the values are the default settings
							wil::com_ptr<ICoreWebView2Settings> settings;
							page_webview->get_Settings(&settings);
							settings->put_IsScriptEnabled(TRUE);
							settings->put_AreDefaultScriptDialogsEnabled(TRUE);
							settings->put_IsWebMessageEnabled(TRUE);

							// Resize WebView to fit the bounds of the parent window
							RECT bounds;
							GetClientRect(hWnd, &bounds);
							page_webviewController->put_Bounds(bounds);

							int wideLen = MultiByteToWideChar(CP_UTF8, 0, url, -1, nullptr, 0);

							wchar_t* wideStr = new wchar_t[wideLen];

							MultiByteToWideChar(CP_UTF8, 0, url, -1, wideStr, wideLen);

							// Schedule an async task to navigate to Bing
							page_webview->Navigate(wideStr);

							delete[] wideStr;


							page_webview->add_WebMessageReceived(
								Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
									[](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
										LPWSTR message;
										args->TryGetWebMessageAsString(&message);

										page_webview->Navigate(message);

										CoTaskMemFree(message);
										return S_OK;
									}).Get(),
										nullptr);

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
}

extern "C" {
	extern int VmStart();
}

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	global_hInstance = hInstance; global_hPrevInstance = hPrevInstance; global_lpCmdLine = lpCmdLine; global_nCmdShow = nCmdShow;
	 
	return VmStart();
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		if (page_webviewController != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			page_webviewController->put_Bounds(bounds);
			
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