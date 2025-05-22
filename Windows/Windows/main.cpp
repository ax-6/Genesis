// Copyright 2025 AQ author, All Rights Reserved.
// This program is licensed under the AQ License. You can find the AQ license in
// the root directory.

#include <iostream>
#include <map>

#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>

#include "WebView2.h"

#include "main.h"

static TCHAR szWindowClass[] = _T("Project Genesis");

static TCHAR szTitle[] = _T("Project Genesis");

HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static wil::com_ptr<ICoreWebView2Controller> page_webviewController;

static wil::com_ptr<ICoreWebView2> page_webview;

std::map<HWND, wil::com_ptr<ICoreWebView2Controller>> g_webViewControllers;
std::map<HWND, wil::com_ptr<ICoreWebView2>> g_webViews;
static wil::com_ptr<ICoreWebView2Environment> g_webViewEnvironment;


HINSTANCE global_hInstance;
HINSTANCE global_hPrevInstance;
LPSTR     global_lpCmdLine;
int       global_nCmdShow;

const char* global_window_class; const char* global_title; int global_length; int global_height; const char* global_url;

LPCWSTR global_url_buffer;

void CreateWebView(HWND hwnd) {
	if (!g_webViewEnvironment) return;

	g_webViewEnvironment->CreateCoreWebView2Controller(
		hwnd,
		Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
			[hwnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
				if (SUCCEEDED(result)) {

					controller->get_CoreWebView2(&g_webViews[hwnd]);

					wil::com_ptr<ICoreWebView2Settings> settings;
					g_webViews[hwnd]->get_Settings(&settings);
					settings->put_IsScriptEnabled(TRUE);
					settings->put_AreDefaultScriptDialogsEnabled(TRUE);
					settings->put_IsWebMessageEnabled(TRUE);

					RECT bounds;
					GetClientRect(hwnd, &bounds);
					controller->put_Bounds(bounds);

					g_webViewControllers[hwnd] = controller;

					//MessageBox(NULL, global_url_buffer, L"Debug", MB_OK);

					g_webViews[hwnd]->Navigate(global_url_buffer);
				}
				return S_OK;
			}).Get());

}



DWORD WINAPI WindowsStartAsync(LPVOID lpParam)
{
	const char* window_class = global_window_class; const char* title= global_title; int length= global_length; int height= global_height; const char* url= global_url;

	HINSTANCE hInstance = GetModuleHandle(NULL);
	HINSTANCE hPrevInstance = global_hPrevInstance;
	LPSTR     lpCmdLine = global_lpCmdLine;
	int       nCmdShow = global_nCmdShow;

	int wc_length = MultiByteToWideChar(CP_UTF8, 0, window_class, -1, NULL, 0);
	wchar_t* wc_str = new wchar_t[wc_length];
	MultiByteToWideChar(CP_UTF8, 0, window_class, -1, wc_str, wc_length);

	int title_length = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
	wchar_t* title_str = new wchar_t[title_length];
	MultiByteToWideChar(CP_UTF8, 0, title, -1, title_str, title_length);


	int url_length = MultiByteToWideChar(CP_UTF8, 0, url, -1, NULL, 0);
	wchar_t* wideString = new wchar_t[url_length];
	MultiByteToWideChar(CP_UTF8, 0, url, -1, wideString, url_length);
	global_url_buffer = wideString;

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
	wcex.lpszClassName = wc_str;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		DWORD error = GetLastError(); 

		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,
			NULL
		);
		MessageBox(NULL,
			(LPCTSTR)lpMsgBuf,
			_T("Windows Desktop Guided Tour"),
			NULL);
		LocalFree(lpMsgBuf);

		return 1;
	}

	hInst = hInstance;

	HWND hWnd = CreateWindow(
		wc_str,
		title_str,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		length, height,
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

	ShowWindow(hWnd,
		nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	global_url_buffer = L"about:blank";

	//UnregisterClass(szWindowClass, hInstance);

	return (int)msg.wParam;
}


extern "C" {
	int CALLBACK WindowsStart(const char* window_class, const char* title, int length, int height, const char* url)
	{
		global_window_class = window_class;
		global_title = title;
		global_length = length;
		global_height = height;
		global_url = url;


		HANDLE hThread = CreateThread(NULL, 0, WindowsStartAsync, NULL, 0, NULL); //WaitForSingleObject(hThread, INFINITE);
		if (hThread != NULL) {
			CloseHandle(hThread);
		}
		return 0;
	}
}


int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	global_hInstance = hInstance; global_hPrevInstance = hPrevInstance; global_lpCmdLine = lpCmdLine; global_nCmdShow = nCmdShow;

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	CreateCoreWebView2EnvironmentWithOptions(
		nullptr, nullptr, nullptr,
		Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
				if (SUCCEEDED(result)) {
					g_webViewEnvironment = env;
				}
				return S_OK;
			}).Get());

	//MessageBox(NULL, L"WinMain called!", L"Debug", MB_OK);

	while (!g_webViewEnvironment);VmStart();
	while (true)Sleep(1000);
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		CreateWebView(hWnd);
		break;

	case WM_SIZE:
		if (g_webViewControllers[hWnd] != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			g_webViewControllers[hWnd]->put_Bounds(bounds);

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