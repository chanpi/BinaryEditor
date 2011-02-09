#include "BinEdit.h"
#include "HugeFile.h"
#include <tchar.h>

#define WRITEBUF_SIZE	1024
#define CELL_H			16
#define CELL_W			9
#define DUMP_OFFSET_X	20
#define DUMP_OFFSET_Y	2
#define ASCII_OFFSET_X	68

extern HINSTANCE hInst;
extern TCHAR szTitle[MAX_LOADSTRING];

BinEdit::BinEdit(void)
{
	fInfo.linesPerPage = 0;
	fInfo.maxLine = 0;
	fInfo.startLine = 0;

	_tcscpy_s(fInfo.fname, _T(""));
	fInfo.cell_H = CELL_H;
	fInfo.cell_W = CELL_W;
}

BinEdit::~BinEdit(void)
{
}

int BinEdit::OpenFile(HWND hWnd, LPCTSTR fname) {
	fInfo.hfile.close();
	fInfo.fname[0] = TEXT('\0');
	UpdateTitle(hWnd);

	if (fInfo.hfile.open(fname) != SUCCESS) {
		MessageBox(hWnd, _T("�w�肳�ꂽ�t�@�C�����J���܂���ł����B"), szTitle, MB_OK | MB_ICONERROR);
		return -1;
	}

	_tcscpy_s(fInfo.fname, fname);
	UpdateTitle(hWnd);

	fInfo.maxLine = (fInfo.hfile.getSize() + 0x10 - 1) / 0x10;

	InvalidateRect(hWnd, NULL, TRUE);
	return 0;
}

int BinEdit::OnOpen(HWND hWnd) {
	TCHAR fname[MAX_PATH] = TEXT("");

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFile = fname;
	ofn.nMaxFile = MAX_PATH;
	if (!GetOpenFileName(&ofn)) {
		return -1;
	}

	return OpenFile(hWnd, fname);
}

int BinEdit::OnSave(HWND hWnd) {
	fInfo.hfile.save();
	return 0;
}

int BinEdit::OnSaveAs(HWND hWnd) {
	// �ʖ��ۑ��p�̃t�@�C�������擾
	TCHAR fname[MAX_PATH] = _T("");
	_tcscpy_s(fname, fInfo.fname);

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = fname;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_OVERWRITEPROMPT;
	if (!GetSaveFileName(&ofn)) {	// OK�ȊO�i�L�����Z���̏ꍇ���j
		return 0;
	}

	// �I�[�v�����̃t�@�C���̃f�[�^�����ׂď�������
	HANDLE hFile = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == NULL) {
		MessageBox(hWnd, _T("�ۑ���̃t�@�C�����J���܂���ł����B"), szTitle, MB_OK|MB_ICONERROR);
		return -1;
	}

	DWORD written = 0;	// �ǂݍ��܂ꂽ�o�C�g��
	BYTE buf[WRITEBUF_SIZE];
	int bufIndex = 0;
	ULONGLONG fileSize = fInfo.hfile.getSize();

	// ���݂̃r���[���ύX����Ă���ꍇ�A���̃r���[�̃f�[�^���ɕۑ�
	// �Ō�܂Ńt�@�C���ɏ����o��
	if (fInfo.hfile.isDirty()) {
		// ���t�@�C���ɂ͕ۑ��s�v�Ȃ̂Ń��Z�b�g
		fInfo.hfile.resetDirty();

		ULONGLONG baseIndex = fInfo.hfile.getBaseIndex();
		LARGE_INTEGER seekpos;
		seekpos.QuadPart = baseIndex;
		SetFilePointerEx(hFile, seekpos, NULL, FILE_BEGIN);
		for (ULONGLONG pos = baseIndex; pos < fileSize; pos++) {
			buf[bufIndex++] = fInfo.hfile.get(pos);
			if (bufIndex >= WRITEBUF_SIZE) {
				WriteFile(hFile, buf, bufIndex, &written, NULL);
				bufIndex = 0;
			}
		}

		if (bufIndex > 0) {
			WriteFile(hFile, buf, bufIndex, &written, NULL);
		}

		// �o�͂����t�@�C���̐擪�ɃV�[�N���āA�c��������o������
		// ���łɏ������������㏑�����Ȃ��悤��fileSize�𒲐�
		seekpos.QuadPart = 0;
		SetFilePointerEx(hFile, seekpos, NULL, FILE_BEGIN);
		fileSize = baseIndex;
		bufIndex = 0;	// �K�v�ȋC������B
	}

	for (ULONGLONG pos = 0; pos < fileSize; pos++) {
		buf[bufIndex++] = fInfo.hfile.get(pos);
		if (bufIndex >= WRITEBUF_SIZE) {
			WriteFile(hFile, buf, bufIndex, &written, NULL);
			bufIndex = 0;
		}
	}
	if (bufIndex > 0) {
		WriteFile(hFile, buf, bufIndex, &written, NULL);
	}
	CloseHandle(hFile);

	return OpenFile(hWnd, fname);
}

int BinEdit::OnExit(HWND hWnd) {
	int rc = 0;
	if (fInfo.hfile.isDirty()) {
		int answer = MessageBox(hWnd, _T("�t�@�C���͕ύX����Ă��܂��B�ۑ����܂����H"), szTitle, MB_YESNOCANCEL|MB_ICONEXCLAMATION);
		switch (answer) {
		case IDYES:
			fInfo.hfile.close(TRUE);
			break;

		case IDNO:
			fInfo.hfile.close(FALSE);
			break;

		case IDCANCEL:
			rc = -1;
			break;
		}
	}
	return rc;
}

int BinEdit::OnPaint(HWND hWnd, HDC hdc) {
	RECT rect;
	GetClientRect(hWnd, &rect);

	if (fInfo.hfile.isLoaded()) {
		ULONGLONG fileSize = fInfo.hfile.getSize();

		int linesPerPage = rect.bottom/fInfo.cell_H - DUMP_OFFSET_Y;

		TCHAR out[BUFFER_SIZE];

		int h = fInfo.cell_H;
		int w = fInfo.cell_W;

		// ����4�r�b�g�̃�������`��
		for (int i = 0; i < 16; i++) {
			_stprintf_s(out, _T("%02X "), i);
			WriteString(hdc, out, DUMP_OFFSET_X + i*3, 0, w, h);
		}

		// ��؂���̕`��
		int linePosX = w * DUMP_OFFSET_X;
		int linePosY = h * DUMP_OFFSET_Y - 4;
		MoveToEx(hdc, linePosX, linePosY, NULL);
		linePosX += (0x10 * 3 -1);
		LineTo(hdc, w * (20+16*3-1), linePosY);

		// g_startLine�̍s����A�P�s�P�U�o�C�g�����ɕ`�悷��
		BOOL cont = TRUE;
		ULONGLONG pos = fInfo.startLine * 0x10;
		for (int line = 0; line < linesPerPage && cont; line++) {
			// �A�h���X�̕\��
			_stprintf_s(out, _T("%016X: "), pos);
			int ypos = line + DUMP_OFFSET_Y;
			WriteString(hdc, out, 1, ypos, w, h);

			// �_���v�f�[�^
			for (int i = 0; i < 16; i++, pos++) {
				int data = fInfo.hfile.get(pos);
				if (data < 0) {
					if (data != ERR_DIRTY_VIEW) {
						cont = FALSE;
						break;
					}
					_tcscpy_s(out, _T("?? "));

				} else {
					_stprintf_s(out, _T("%02X "), data);
				}
				WriteString(hdc, out, DUMP_OFFSET_X + i*3, ypos, w, h);

				// �Ή�����ASCII�����̕`��
				TCHAR ch = data;
				if (_istprint(ch)) {
					out[0] = ch;
				} else if (data >= 0) {
					out[0] = _T('.');
				} else {
					out[0] = _T('?');
				}
				out[1] = _T('\0');
				WriteString(hdc, out, ASCII_OFFSET_X + i, ypos, w, h);
			}
		}
	} else {
		// �t�@�C�����J���Ă��Ȃ��ꍇ�́A�N���C�A���g�̈�S�̂�w�i�F�œh��Ԃ�
		FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));
	}
	return 0;
}

int BinEdit::OnKeyDown(HWND hWnd, WPARAM wParam) {
	return 0;
}



/////////////////////////////////////////////////////////////

void BinEdit::UpdateTitle(HWND hWnd) {
	TCHAR title[MAX_PATH];
	if (fInfo.fname[0] != _T('\0')) {
		_stprintf_s(title, _T("%s - %s"), fInfo.fname, szTitle);
	} else {
		_tcscpy_s(title, szTitle);
	}

	SetWindowText(hWnd, title);
}

void BinEdit::WriteString(HDC hdc, LPCTSTR str, int x, int y, int width, int height) {
	if (y == 0) {
		y = height / 2;	// �P�s�ڂ�������������悤�ɒ���
	} else {
		y *= height;
	}

	RECT rect;
	int len = _tcslen(str);
	rect.left = x * width;
	rect.right = rect.left + len * width;
	rect.top = y;
	rect.bottom = y + height;
	FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));

	while (*str != _T('\0')) {
		TextOut(hdc, x * width, y, str++, 1);
		x++;
	}
}
