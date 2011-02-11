#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include "Defs.h"
#include "BinEdit.h"
#include "resource.h"

HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];			// タイトルバーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];	// メインウィンドウクラス

PRINTDLGEX printDlgEx;

static BinEdit binEdit;

static ATOM MyRegisterClass(HINSTANCE hInstance);
static BOOL InitInstance(HINSTANCE, int);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// 二重起動防止

	hInst = hInstance;

	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列の初期化
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BINEDIT, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow)) {
		MessageBox(NULL, TEXT("[ERROR] CreateWindow"), szTitle, MB_OK);
		return EXIT_FAILURE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
		
	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszClassName = szWindowClass;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

	return RegisterClass(&wc);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	HWND hWnd;

	hWnd = CreateWindow(szWindowClass, szTitle,
		WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL, 
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
		NULL, NULL, hInstance, NULL);

	if (hWnd == NULL) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam) {

	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (umsg) {
	case WM_CREATE:
		binEdit.OnCreate(hWnd);
		break;

	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId) {
		case ID_OPEN:
			binEdit.OnOpen(hWnd);
			break;

		case ID_SAVE:
			binEdit.OnSave(hWnd);
			break;

		case ID_SAVEAS:
			binEdit.OnSaveAs(hWnd);
			break;

		case ID_PRINT:
			binEdit.OnPrint(hWnd);
			break;

		case ID_QUIT:
			binEdit.OnExit(hWnd);
			PostQuitMessage(0);
			break;

		case ID_FONT:
			binEdit.OnFont(hWnd);
			break;

		case ID_HELP_VERSION:
			break;

		default:
			return DefWindowProc(hWnd, umsg, wParam, lParam);
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		binEdit.OnPaint(hWnd, hdc);
		EndPaint(hWnd, &ps);
		break;

	case WM_KEYDOWN:
		binEdit.OnKeyDown(hWnd, wParam);
		break;

	case WM_MOUSEWHEEL:
		if (GET_WHEEL_DELTA_WPARAM(wParam) < 0) {
			binEdit.OnVScroll(hWnd, SB_LINEDOWN);
		} else {
			binEdit.OnVScroll(hWnd, SB_LINEUP);
		}
		break;

	case WM_VSCROLL:
		binEdit.OnVScroll(hWnd, wParam);
		break;

	case WM_HSCROLL:
		binEdit.OnHScroll(hWnd, wParam);
		break;

	case WM_SIZE:
		binEdit.OnSize(hWnd, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_DESTROY:
		binEdit.OnExit(hWnd);
		break;
		
	default:
		return DefWindowProc(hWnd, umsg, wParam, lParam);
		
	}
	return 0;
}
