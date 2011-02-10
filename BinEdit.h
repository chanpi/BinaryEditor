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
	int requiredWidth;
	int ofsX;
	int cursorX;
	int cursorY;
	BOOL cursorEnabled;
} FileInfo;

class BinEdit
{
public:
	BinEdit(void);
	~BinEdit(void);

	int OpenFile(HWND hWnd, LPCTSTR fname);
	int OnOpen(HWND hWnd);
	int OnSave(HWND hWnd);
	int OnSaveAs(HWND hWnd);
	int OnExit(HWND hWnd);

	int OnPaint(HWND hWnd, HDC hdc);
	int OnKeyDown(HWND hWnd, UINT vKey);
	int OnVScroll(HWND hWnd, WORD type);
	int OnHScroll(HWND hWnd, WORD type);

	int OnSize(HWND hWnd, WORD width, WORD height);

private:
	FileInfo fInfo;		// タブなどを使って複数のファイルを管理したい場合は配列などでもつ
	void UpdateTitle(HWND hWnd);
	void WriteString(HDC hdc, LPCTSTR str, int x, int y, int width, int height);
	int QuerySaveChanges(HWND hWnd);
	void ScrollUpDown(HWND hWnd, int lines, BOOL scrollCursor = FALSE);
	void ReverseColor(HDC hdc);
	void AdjustCursor(void);
	void EnableCursor(HWND hWnd, BOOL enable);
};