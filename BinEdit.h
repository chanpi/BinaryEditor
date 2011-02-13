#pragma once

#include <Windows.h>
#include "Defs.h"
#include "HugeFile.h"

typedef struct {
	HugeFile hfile;
	TCHAR fname[MAX_PATH];
	ULONGLONG startLine;
	ULONGLONG maxLine;
	int lastXatLastLine;
	int linesPerPage;
	int cell_H;
	int cell_W;
	int cursorX;
	int cursorY;
	BOOL cursorEnabled;
	HFONT hMainFont;
	LOGFONT lf;	// メインで使う論理フォントに対するLOGFONT構造体
				// フォントダイアログ、現在のフォントの設定として初期化に使う
} FileInfo;

class BinEdit
{
public:
	BinEdit(void);
	~BinEdit(void);

	int OnCreate(HWND hWnd);
	int OpenFile(HWND hWnd, LPCTSTR fname);
	int OnOpen(HWND hWnd);
	int OnSave(HWND hWnd);
	int OnSaveAs(HWND hWnd);
	int OnPrint(HWND hWnd);
	int OnExit(HWND hWnd);
	int OnFont(HWND hWnd);

	int OnPaint(HWND hWnd, HDC hdc);
	int OnKeyDown(HWND hWnd, UINT vKey);
	int OnVScroll(HWND hWnd, WORD type);
	int OnHScroll(HWND hWnd, WORD type);

	int OnRButtonDown(HWND hWnd, UINT x, UINT y);

	int OnSize(HWND hWnd, WORD width, WORD height);

	int OnDestroy(HWND hWnd);

	int char2hex(TCHAR ch);
	BOOL SearchNext(HWND hDlg, int hexByte, BOOL forward);


private:
	FileInfo fInfo;		// タブなどを使って複数のファイルを管理したい場合は配列などでもつ
	void UpdateTitle(HWND hWnd);
	void WriteString(HDC hdc, LPCTSTR str, int x, int y, int width, int height, int ofsX);
	int QuerySaveChanges(HWND hWnd);
	void ScrollUpDown(HWND hWnd, int lines, BOOL scrollCursor = FALSE);
	void ReverseColor(HDC hdc);
	void AdjustCursor(void);
	void EnableCursor(HWND hWnd, BOOL enable);

	void CreateMainFont(HWND hWnd);
	int CalcCellWidth(HDC hdc);
	void GetCellRect(RECT* pRect, int x, int y);
	void SetCellSize(int width, int height);
};