#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>

#include "WebView2.h"

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

static wil::com_ptr<ICoreWebView2Controller> page_webviewController;
static wil::com_ptr<ICoreWebView2> page_webview;

void InjectNavigationScript()
{
	if (!page_webview) return;

	// 导航栏 HTML 和 CSS
	const wchar_t* htmlScript = LR"(
        window.addEventListener('DOMContentLoaded', function() {
            const navBar = document.createElement('div');
            navBar.id = 'custom-navbar';
            navBar.style.position = 'fixed';
            navBar.style.top = '0';
            navBar.style.left = '0';
            navBar.style.width = '100%';
            navBar.style.height = '50px';
            navBar.style.backgroundColor = '#f0f0f0';
            navBar.style.zIndex = '9999';
            navBar.style.display = 'flex';
            navBar.style.alignItems = 'center';
            navBar.style.boxShadow = '0 2px 4px rgba(0,0,0,0.1)';

            const input = document.createElement('input');
            input.id = 'url-input';
            input.type = 'text';
            input.placeholder = '输入网址';
            input.style.marginLeft = '10px';
            input.style.padding = '5px';
            input.style.width = '300px';

            const goButton = document.createElement('button');
            goButton.textContent = '跳转';
            goButton.style.marginLeft = '5px';
            goButton.style.padding = '5px 10px';
            goButton.onclick = function() {
                const url = input.value;
                if (url) {
                    window.chrome.webview.postMessage(`navigate|${url}`);
                }
            };

            const settingsButton = document.createElement('button');
            settingsButton.textContent = '设置';
            settingsButton.style.marginLeft = 'auto';
            settingsButton.style.marginRight = '10px';
            settingsButton.onclick = function() {
                window.chrome.webview.postMessage('open-settings');
            };

            const closeButton = document.createElement('button');
            closeButton.textContent = '退出';
            closeButton.style.marginRight = '10px';
            closeButton.onclick = function() {
                window.chrome.webview.postMessage('close');
            };

            navBar.appendChild(input);
            navBar.appendChild(goButton);
            navBar.appendChild(settingsButton);
            navBar.appendChild(closeButton);

            document.body.insertBefore(navBar, document.body.firstChild);

            document.body.style.marginTop = '50px';
        })();
    )";

	// 注入脚本
	page_webview->AddScriptToExecuteOnDocumentCreated(htmlScript, nullptr);
}

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
		Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(hWnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewController = controller;
							webviewController->get_CoreWebView2(&webview);
						}

						// Add a few settings for the webview
						// The demo step is redundant since the values are the default settings
						wil::com_ptr<ICoreWebView2Settings> settings;
						webview->get_Settings(&settings);
						settings->put_IsScriptEnabled(TRUE);
						settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						settings->put_IsWebMessageEnabled(TRUE);

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(hWnd, &bounds);
						bounds.bottom = 50;
						webviewController->put_Bounds(bounds);

						// Schedule an async task to navigate to Bing
						webview->Navigate(L"file:///F:/Genesis/Windows/Windows/main.html");

						webview->add_WebMessageReceived(
							Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
								[](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
									LPWSTR message;
									args->TryGetWebMessageAsString(&message);

									page_webview->Navigate(message);

									// 释放内存
									CoTaskMemFree(message);
									return S_OK;
								}).Get(),
									nullptr);

						return S_OK;

					}).Get());

				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(hWnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
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
						bounds.top = 50;
						page_webviewController->put_Bounds(bounds);

						InjectNavigationScript();

						// Schedule an async task to navigate to Bing
						page_webview->Navigate(L"https://www.baidu.com");



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
		if (page_webviewController != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			RECT page_bounds = bounds;

			bounds.bottom = 50;
			webviewController->put_Bounds(bounds);

			page_bounds.top = 50;
			page_webviewController->put_Bounds(page_bounds);
			
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