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
static HWND hSearchDlg = 0;

static ATOM MyRegisterClass(HINSTANCE hInstance);
static BOOL InitInstance(HINSTANCE, int);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK SearchDlgProc(HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam);

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
		if (hSearchDlg == NULL || !IsDialogMessage(hSearchDlg, &msg)) {
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
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
	int x = CW_USEDEFAULT, y = 0;
	int w = CW_USEDEFAULT, h = 0;
	
	HKEY hKey;		// 書きこむレジストリ・キーのハンドル
	if (RegCreateKeyEx(HKEY_CURRENT_USER, MY_REGKEY, 0, NULL, 0,
		KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {

		LONG rc;
		DWORD type;		// 値の型を取得（DWORD）を想定

		// x座標読み込み
		DWORD size = sizeof(DWORD);		// 値のバイト・サイズ
		rc = RegQueryValueEx(hKey, REGVAL_X, 0, &type, (BYTE*)&x, &size);
		// y座標読み込み
		if (rc == ERROR_SUCCESS) {
			size = sizeof(DWORD);		// 値のバイト・サイズ
			rc = RegQueryValueEx(hKey, REGVAL_Y, 0, &type, (BYTE*)&y, &size);
		}
		if (rc == ERROR_SUCCESS) {
			size = sizeof(DWORD);		// 値のバイト・サイズ
			rc = RegQueryValueEx(hKey, REGVAL_W, 0, &type, (BYTE*)&w, &size);
		}
		if (rc == ERROR_SUCCESS) {
			size = sizeof(DWORD);		// 値のバイト・サイズ
			rc = RegQueryValueEx(hKey, REGVAL_H, 0, &type, (BYTE*)&h, &size);
		}
		RegCloseKey(hKey);

		if (rc == ERROR_SUCCESS) {
			RECT rect;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
			if (x < rect.left) {
				x = rect.left;
			}
			if (y < rect.top) {
				y = rect.top;
			}
			if (w < 0 || rect.right - rect.left < w) {
				w = (rect.right - rect.left) / 2;
			}
			if (h < 0 || rect.bottom - rect.top < h) {
				h = (rect.bottom - rect.top) / 2;
			}
			if (x + w > rect.right) {
				x = rect.right - w;
			}
			if (y + h > rect.bottom) {
				y = rect.bottom - h;
			}
		}
	}

	hWnd = CreateWindow(szWindowClass, szTitle,
		WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL, 
		x, y, w, h, 
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
			if (!binEdit.OnExit(hWnd)) {
				DestroyWindow(hWnd);
			}
			break;

		case ID_SEARCH:
			hSearchDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SEARCHDIALOG), hWnd, (DLGPROC)SearchDlgProc);
			break;

		case ID_FONT:
			binEdit.OnFont(hWnd);
			break;

		case ID_HELP_VERSION:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTDIALOG), hWnd, (DLGPROC)AboutDlgProc);
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

	//case WM_DROPFILES:
	//	MessageBox(hWnd, (TCHAR*)lParam, szTitle, MB_OK);
	//	break;

	case WM_RBUTTONDOWN:
		binEdit.OnRButtonDown(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
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
		binEdit.OnVScroll(hWnd, LOWORD(wParam));
		break;

	case WM_HSCROLL:
		binEdit.OnHScroll(hWnd, LOWORD(wParam));
		break;

	case WM_SIZE:
		binEdit.OnSize(hWnd, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_DESTROY:
		binEdit.OnExit(hWnd);
		binEdit.OnDestroy(hWnd);
		PostQuitMessage(0);

		break;
		
	default:
		return DefWindowProc(hWnd, umsg, wParam, lParam);
		
	}
	return 0;
}

INT_PTR CALLBACK SearchDlgProc(HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);

	switch (umsg) {
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_FINDDOWN:
		case IDC_BUTTON_FINDUP:
			{
				TCHAR buf[48] = {0};
				UINT num = GetDlgItemText(hDlg, IDC_EDIT1, buf, sizeof(buf)/sizeof(buf[0]));
				if (num == 2) {
					int hex1st = binEdit.char2hex(buf[0]);
					int hex2nd = binEdit.char2hex(buf[1]);

					if (hex1st < 0 || hex2nd < 0) {
						MessageBox(hDlg, _T("検索データの指定が正しくありません。2桁の16進数で指定してください。"), szTitle, MB_OK | MB_ICONWARNING);
					} else {
						int hexByte = hex1st << 4 | hex2nd;
						binEdit.SearchNext(hDlg, hexByte, LOWORD(wParam) == IDC_BUTTON_FINDDOWN);
					}
				} else {
					MessageBox(hDlg, _T("今のところ1バイトデータのみ検索可能です\r\n")
						_T("2桁の16進数で指定してください。"), szTitle, MB_OK | MB_ICONWARNING);
				}
			}
			return TRUE;
			//break;

		case IDCANCEL:
			DestroyWindow(hDlg);
			hSearchDlg = NULL;
			return (INT_PTR)TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT umsg, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (umsg) {
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}