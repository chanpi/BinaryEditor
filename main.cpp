#include <Windows.h>
#include <WindowsX.h>

#define APP_NAME	TEXT("HugeFile")

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// “ñd‹N“®–hŽ~

	HWND hWnd;
	WNDCLASS wc;
	MSG msg;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszClassName = APP_NAME;
	wc.lpszMenuName = NULL;

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, TEXT("[ERROR] RegisterClass"), APP_NAME, MB_OK);
		return EXIT_FAILURE;
	}

	hWnd = CreateWindow(APP_NAME, TEXT("Binary Editor"),
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
		NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		MessageBox(NULL, TEXT("[ERROR] CreateWindow"), APP_NAME, MB_OK);
		return EXIT_FAILURE;
	}

	ShowWindow(hWnd, SW_SHOW);

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
		
	return EXIT_SUCCESS;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
	switch (umsg) {
	case WM_DESTROY:
		PostQuitMessage(EXIT_SUCCESS);

	default:
		return DefWindowProc(hWnd, umsg, wParam, lParam);
		
	}
	return 0;
}
