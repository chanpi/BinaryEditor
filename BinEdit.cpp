#include "BinEdit.h"
#include "HugeFile.h"
#include <tchar.h>

#define WRITEBUF_SIZE	1024
#define CELL_H			16
#define CELL_W			9
#define DUMP_OFFSET_X	20
#define DUMP_OFFSET_Y	2
#define ASCII_OFFSET_X	(DUMP_OFFSET_X+50)
#define MAX_COLUMN		(ASCII_OFFSET_X+16+1)

extern HINSTANCE hInst;
extern TCHAR szTitle[MAX_LOADSTRING];
extern PRINTDLGEX printDlgEx;

BinEdit::BinEdit(void)
{
	fInfo.linesPerPage = 0;
	fInfo.maxLine = 0;
	fInfo.startLine = 0;

	fInfo.lastXatLastLine = 0;

	_tcscpy_s(fInfo.fname, _T(""));
	fInfo.cell_H = CELL_H;
	fInfo.cell_W = CELL_W;

	fInfo.cursorX = 0;
	fInfo.cursorY = 0;
	fInfo.cursorEnabled = TRUE;

	fInfo.hMainFont = NULL;
}

BinEdit::~BinEdit(void)
{
}


int BinEdit::OnCreate(HWND hWnd) {
	ZeroMemory(&printDlgEx, sizeof(printDlgEx));
	CreateMainFont(hWnd);
	return 0;
}

int BinEdit::OpenFile(HWND hWnd, LPCTSTR fname) {
	fInfo.hfile.close(FALSE);
	fInfo.fname[0] = TEXT('\0');
	UpdateTitle(hWnd);

	if (fInfo.hfile.open(fname) != SUCCESS) {
		MessageBox(hWnd, _T("�w�肳�ꂽ�t�@�C�����J���܂���ł����B"), szTitle, MB_OK | MB_ICONERROR);
		return -1;
	}

	_tcscpy_s(fInfo.fname, fname);
	UpdateTitle(hWnd);

	// �J�[�\���������ʒu�֖߂�
	fInfo.cursorX = fInfo.cursorY = 0;

	// ���������̃X�N���[���o�[�ݒ�
	fInfo.startLine = 0;
	fInfo.maxLine = (fInfo.hfile.getSize() + 0x10 -1) / 0x10;
	fInfo.lastXatLastLine = (int)(fInfo.hfile.getSize() % 0x10);
	if (!fInfo.lastXatLastLine) {
		fInfo.lastXatLastLine = 0x10;
	}

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE;
	si.nPos = 0;
	si.nMin = 1;
	si.nMax = (int)fInfo.maxLine;
	SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

	RECT rect;
	GetClientRect(hWnd, &rect);
	OnSize(hWnd, (WORD)rect.right, (WORD)rect.bottom);

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

int BinEdit::OnSave(HWND /*hWnd*/) {
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
			buf[bufIndex++] = (BYTE)fInfo.hfile.get(pos);
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
		buf[bufIndex++] = (BYTE)fInfo.hfile.get(pos);
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

int BinEdit::OnPrint(HWND hWnd) {
	if (!fInfo.hfile.isLoaded()) {
		return 0;
	}

	// PRINTDLGEX�\���̂̏�����
	if (printDlgEx.hwndOwner == NULL) {
		// ����̂�Flags��������
		printDlgEx.Flags = PD_RETURNDC | PD_CURRENTPAGE
			| PD_USEDEVMODECOPIESANDCOLLATE
			| PD_NOPAGENUMS | PD_NOSELECTION;
	}
	printDlgEx.lStructSize = sizeof(printDlgEx);
	printDlgEx.hwndOwner = hWnd;
	printDlgEx.nStartPage = START_PAGE_GENERAL;
	printDlgEx.dwResultAction = 0;

	// ����_�C�A���O�̌Ăяo��
	if (PrintDlgEx(&printDlgEx) == S_OK) {
		if (printDlgEx.dwResultAction == PD_RESULT_PRINT) {

			// ����W���u�̖��O��ݒ�
			TCHAR docName[BUFFER_SIZE] = {0};
			GetWindowText(hWnd, docName, sizeof(docName)/sizeof(docName[0]));
			DOCINFO di;
			ZeroMemory(&di, sizeof(di));
			di.cbSize = sizeof(di);
			di.lpszDocName = docName;

			// �E�B���h�E��DC�̏c�����̉𑜓x�iDPI�j���擾
			HDC hdc = GetDC(hWnd);
			int winExt = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(hWnd, hdc);

			// �v�����^DC�̏c�����̉𑜓x�iDPI�j���擾
			int viewExt = GetDeviceCaps(printDlgEx.hDC, LOGPIXELSY);

			// �E�B���h�E��1�C���`�̃h�b�g�����v�����^��1�C���`�̃h�b�g���ɑΉ�����悤�Ƀ}�b�s���O���[�h��ݒ�
			SetMapMode(printDlgEx.hDC, MM_ISOTROPIC);
			SetWindowExtEx(printDlgEx.hDC, winExt, winExt, NULL);
			SetViewportExtEx(printDlgEx.hDC, viewExt, viewExt, NULL);

			// ����J�n
			if (StartDoc(printDlgEx.hDC, &di) > 0) {
				if (StartPage(printDlgEx.hDC) > 0) {
					// ���ʕ`�惋�[�`�����Ăяo��
					OnPaint(hWnd, printDlgEx.hDC);
					if (EndPage(printDlgEx.hDC) > 0) {
						EndDoc(printDlgEx.hDC);
					}
				}

				DeleteDC(printDlgEx.hDC);
			}
		}
	}
	return 0;
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

	PostQuitMessage(0);
	DeleteObject(fInfo.hMainFont);
	return rc;
}

int BinEdit::OnFont(HWND hWnd) {
	CHOOSEFONT cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &fInfo.lf;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;	
	// CF_INITTOLOGFONTSTRUCT�͎w�肵��LOGFONT�\���̂Ńt�H���g�_�C�A���O������������Ƃ��Ɏw��

	// �t�H���g�_�C�A���O��\��
	if (ChooseFont(&cf)) {
		// ����ꂽLOGFONT�\���̂ŁA�_���t�H���g���쐬
		HFONT hFont = CreateFontIndirect(&fInfo.lf);

		if (hFont != NULL) {

			// �Â��_���t�H���g���폜
			if (fInfo.hMainFont != NULL && fInfo.hMainFont != hFont) {
				DeleteObject(fInfo.hMainFont);
			}
			fInfo.hMainFont = hFont;

			// �t�H���g�̃T�C�Y����Z���̃T�C�Y���v�Z
			HDC hdc;
			hdc = GetDC(hWnd);
			HFONT hOldFont = (HFONT)SelectObject(hdc, fInfo.hMainFont);

			TEXTMETRIC tm;
			GetTextMetrics(hdc, &tm);
			SetCellSize(CalcCellWidth(hdc), tm.tmHeight);

			SelectObject(hdc, hOldFont);
			ReleaseDC(hWnd, hdc);


			// �t�H���g���ς�����̂ŃX�N���[���o�[�̐ݒ���X�V
			RECT rect;
			GetClientRect(hWnd, &rect);
			OnSize(hWnd, (WORD)rect.right, (WORD)rect.bottom);

			// �J�[�\�����E�B���h�E���ɂȂ�悤�ɒ���
			EnableCursor(hWnd, FALSE);
			AdjustCursor();
			EnableCursor(hWnd, TRUE);

			InvalidateRect(hWnd, NULL, TRUE);
		} else {
			MessageBox(hWnd, _T("�t�H���g�̕ύX�Ɏ��s���܂����B"), szTitle, MB_OK | MB_ICONERROR);
		}
	}
	return 0;
}

int BinEdit::OnPaint(HWND hWnd, HDC hdc) {
	HFONT hOldFont;

	// �`��悪�f�B�X�v���CDC���ǂ���
	BOOL isDisplay = (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY);

	if (fInfo.hfile.isLoaded()) {
		// �e�L�X�g�`�掞�A�����̔w�i�𓧖��ɂ���
		// �Z������͂ݏo���ĕ`�悳��Ă��镔���������Ȃ����߂ɕK�v
		int prevBkMode = SetBkMode(hdc, TRANSPARENT);

		// ���C���t�H���g���������쐬����Ă���ΐݒ�
		if (fInfo.hMainFont != NULL) {
			hOldFont = (HFONT)SelectObject(hdc, fInfo.hMainFont);
		}

		TCHAR out[BUFFER_SIZE];

		// �����X�N���[���ʒu�̐ݒ�i�������0�j
		int ofsX = 0;		// FileInfo�����o��ofsX�͔p�~
		if (isDisplay) {
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			GetScrollInfo(hWnd, SB_HORZ, &si);
			ofsX = si.nPos;
		}

		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);
		int h = tm.tmHeight;
		int w = CalcCellWidth(hdc);


		// ����4�r�b�g�̃�������`��
		for (int i = 0; i < 16; i++) {
			_stprintf_s(out, _T("%02X "), i);
			WriteString(hdc, out, DUMP_OFFSET_X + i*3, 0, w, h, ofsX);
		}

		// ��؂���̕`��
		int linePosX = w * DUMP_OFFSET_X - ofsX;
		int linePosY = h * DUMP_OFFSET_Y - 4;
		MoveToEx(hdc, linePosX, linePosY, NULL);
		linePosX += w * (0x10 * 3 - 1);
		LineTo(hdc, linePosX, linePosY);

		// ������̂݁A�`�悷��s�����v�Z
		int linesPerPage = fInfo.linesPerPage;
		//int linesPerPage = rect.bottom/fInfo.cell_H - DUMP_OFFSET_Y;
		if (!isDisplay) {
			// VERTRES�ŁA����\�͈͂̍������擾
			POINT pt[1];
			pt[0].x = 0;
			pt[0].y = GetDeviceCaps(hdc, VERTRES);
			DPtoLP(hdc, pt, 1);		// �f�o�C�X���W����_�����W�֕ϊ�
			linesPerPage = pt[0].y / h - DUMP_OFFSET_Y;
		}

		// g_startLine�̍s����A�P�s�P�U�o�C�g�����ɕ`�悷��
		BOOL cont = TRUE;
		ULONGLONG pos = fInfo.startLine * 0x10;
		for (int line = 0; line < linesPerPage && cont; line++) {
			for (int i = 0; i < MAX_COLUMN; i++) {
				out[i] = _T(' ');
			}
			out[MAX_COLUMN] = _T('\0');
			TCHAR buf[100];
			// �A�h���X�̕\��
			_stprintf_s(buf, _T("%016X: "), pos);
			_tcsncpy(out, buf, 18);
			int ypos = line + DUMP_OFFSET_Y;

			// �_���v�f�[�^
			for (int i = 0; i < 16; i++, pos++) {
				int data = fInfo.hfile.get(pos);
				if (data < 0) {
					if (data != ERR_DIRTY_VIEW) {
						cont = FALSE;
						break;
					}
					//_tcsncpy_s(out + DUMP_OFFSET_X + i*3, sizeof(out)/sizeof(out[0]) - DUMP_OFFSET_X - i*3, _T("?? "), 3);
					_tcsncpy(out + DUMP_OFFSET_X + i*3, _T("?? "), 3);

				} else {
					_stprintf_s(buf, _T("%02X "), data);
					//_tcsncpy_s(out + DUMP_OFFSET_X + i*3, sizeof(out)/sizeof(out[0]) - DUMP_OFFSET_X - i*3, buf, 3);
					_tcsncpy(out + DUMP_OFFSET_X + i*3, buf, 3);
				}

				// �Ή�����ASCII�����̕`��
				TCHAR ch = (TCHAR)data;
				if (_istprint(ch)) {
					out[ASCII_OFFSET_X + i] = ch;
				} else if (data >= 0) {
					out[ASCII_OFFSET_X + i] = _T('.');
				} else {
					out[ASCII_OFFSET_X + i] = _T('?');
				}
			}
			WriteString(hdc, out, 0, ypos, w, h, ofsX);
		}

		// �f�B�X�v���C���̂݃J�[�\���̕`��i���]���邾���j
		if (isDisplay && fInfo.cursorEnabled) {
			int data = fInfo.hfile.get((fInfo.startLine + fInfo.cursorY) * 0x10 + fInfo.cursorX / 2);

			if ((fInfo.cursorX & 0x01) == 0) {
				data >>= 4;	// ���4�o�C�g
			} else {
				data &= 0x0F;
			}
			_stprintf_s(out, _T("%X"), data);

			// �J�[�\���ʒu�̃Z���̋�`���擾���āA�����h��Ԃ�
			RECT rect;
			GetCellRect(&rect, fInfo.cursorX, fInfo.cursorY);
			FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

			ReverseColor(hdc);
			int posX = fInfo.cursorX + fInfo.cursorX/2 + DUMP_OFFSET_X;
			WriteString(hdc, out, posX, fInfo.cursorY + DUMP_OFFSET_Y, w, h, ofsX);
			ReverseColor(hdc);
		}
		SetBkMode(hdc, prevBkMode);
		
		if (hOldFont != NULL) {
			SelectObject(hdc, hOldFont);
		}

	} else {
		// �t�@�C�����J���Ă��Ȃ��ꍇ�́A�N���C�A���g�̈�S�̂�w�i�F�œh��Ԃ�
		RECT rect;
		GetClientRect(hWnd, &rect);
		FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));
	}
	return 0;
}

int BinEdit::OnKeyDown(HWND hWnd, UINT vKey) {
	if (!fInfo.hfile.isLoaded()) {
		return 0;
	}

	EnableCursor(hWnd, FALSE);
	switch (vKey) {
	case VK_PRIOR:
		OnVScroll(hWnd, SB_PAGEUP);
		break;

	case VK_NEXT:
		OnVScroll(hWnd, SB_PAGEDOWN);
		break;

	case VK_HOME:
		OnHScroll(hWnd, SB_PAGELEFT);
		break;

	case VK_END:
		OnHScroll(hWnd, SB_PAGERIGHT);
		break;


	case VK_DOWN:
		if (fInfo.cursorY < fInfo.linesPerPage - 1) {
			fInfo.cursorY++;
			AdjustCursor();
		} else {
			OnVScroll(hWnd, SB_LINEDOWN);
		}
		break;

	case VK_UP:
		if (fInfo.cursorY > 0) {
			fInfo.cursorY--;
		} else {
			OnVScroll(hWnd, SB_LINEUP);
		}
		break;

	case VK_LEFT:
		if (fInfo.cursorX > 0) {
			fInfo.cursorX--;
		} else {
			if (fInfo.cursorY > 0) {
				fInfo.cursorY--;
				fInfo.cursorX = 0x10 * 2 - 1;
			} else if (fInfo.startLine > 0) {
				fInfo.cursorX = 0x10 * 2 - 1;
				fInfo.cursorY = 0;
				ScrollUpDown(hWnd, -1);
			}
		}
		break;

	case VK_RIGHT:
		fInfo.cursorX++;
		if (fInfo.cursorX >= 0x10 * 2) {
			if (fInfo.cursorY < fInfo.linesPerPage - 1) {
				fInfo.cursorY++;
				fInfo.cursorX = 0;
			} else if (fInfo.startLine < fInfo.maxLine - fInfo.linesPerPage) {
				fInfo.cursorX = 0;
				ScrollUpDown(hWnd, 1);
			}
		}
		AdjustCursor();
		break;

	default:
		{
			int nibble = -1;
			if ('0' <= vKey && vKey <= '9') {
				nibble = vKey - '0';
			} else if ('A' <= vKey && vKey <= 'F') {
				nibble = vKey - 'A' + 10;
			}

			if (nibble >= 0) {
				ULONGLONG index = (fInfo.startLine + fInfo.cursorY) * 0x10 + fInfo.cursorX / 2;
				int data = fInfo.hfile.get(index);
				if ((fInfo.cursorX & 0x01) == 0) {	// �����ԖځA�܂�J�[�\������ʃo�C�g�̈ʒu�ɂ���Ƃ�
					data = (data & 0x0F) | (nibble << 4);
				} else {
					data = (data & 0xF0) | nibble;
				}
				fInfo.hfile.set(index, data);

				// �A�����͉\�Ȃ悤�ɁA�J�[�\�����E�ɂP�ړ�
				OnKeyDown(hWnd, VK_RIGHT);
			}
		}
		break;
	}
	EnableCursor(hWnd, TRUE);
	return 0;
}

int BinEdit::OnVScroll(HWND hWnd, WORD type) {
	if (!fInfo.hfile.isLoaded()) {
		return 0;
	}

	RECT rect;
	GetClientRect(hWnd, &rect);
	//fInfo.linesPerPage = rect.bottom / fInfo.cell_H - DUMP_OFFSET_Y;

	// �X�N���[����̕\���J�n�s���v�Z����
	switch (type) {
	case SB_LINEDOWN:
		ScrollUpDown(hWnd, 1);
		break;

	case SB_LINEUP:
		ScrollUpDown(hWnd, -1);
		break;

	case SB_PAGEDOWN:
		ScrollUpDown(hWnd, fInfo.linesPerPage);
		break;

	case SB_PAGEUP:
		ScrollUpDown(hWnd, -fInfo.linesPerPage);
		break;

	case SB_THUMBTRACK:
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_TRACKPOS;
			GetScrollInfo(hWnd, SB_VERT, &si);
			ULONGLONG startLine = si.nTrackPos - 1;
			ScrollUpDown(hWnd, (int)(startLine - fInfo.startLine));	// �T����SetScrollInfo�ōĕ`�悳���
		}
		break;
	}

	return 0;
}

int BinEdit::OnHScroll(HWND hWnd, WORD type) {
	if (!fInfo.hfile.isLoaded()) {
		return 0;
	}

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_HORZ, &si);

	int orgPos = si.nPos;

	switch (type) {
	case SB_LINELEFT:
		si.nPos -= fInfo.cell_W;
		if (si.nPos < 0) {
			si.nPos = 0;
		}
		break;

	case SB_LINERIGHT:
		si.nPos += fInfo.cell_W;
		if (si.nPos + (int)si.nPage > si.nMax) {
			si.nPos = si.nMax - (si.nPage - 1);
		}
		break;

	case SB_PAGELEFT:
		si.nPos -= si.nPage;
		if (si.nPos < 0) {
			si.nPos = 0;
		}
		break;

	case SB_PAGERIGHT:
		si.nPos += si.nPage;
		if (si.nPos + (int)si.nPage > si.nMax) {
			si.nPos = si.nMax - (si.nPage - 1);
		}
		break;

	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	}

	si.fMask = SIF_POS;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

	RECT rect;
	GetClientRect(hWnd, &rect);
	ScrollWindowEx(hWnd, orgPos - si.nPos, 0, &rect, &rect, NULL, NULL, SW_ERASE | SW_INVALIDATE);

	return 0;
}

int BinEdit::OnSize(HWND hWnd, WORD width, WORD height) {
	if (!fInfo.hfile.isLoaded()) {
		return 0;
	}

	// �����X�N���[���o�[�̒���
	fInfo.linesPerPage = height / fInfo.cell_H - DUMP_OFFSET_Y;

	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE;
	si.nPage = fInfo.linesPerPage;
	SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

	// �����X�N���[���o�[�̒���
	si.fMask = SIF_PAGE | SIF_RANGE;
	si.nPage = width;
	si.nMin = 0;
	si.nMax = fInfo.cell_W * MAX_COLUMN;
	SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
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

void BinEdit::WriteString(HDC hdc, LPCTSTR str, int x, int y, int width, int height, int ofsX) {
	if (y == 0) {
		y = height / 2;	// �P�s�ڂ�������������悤�ɒ���
	} else {
		y *= height;
	}

	RECT rect;
	int len = _tcslen(str);
	if (len > 1) {
		rect.left = x * width - ofsX;
		rect.right = rect.left + len * width;
		rect.top = y;
		rect.bottom = y + height;
		FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));
	}

	// �������Z���̒����i���������j�ɂ���悤�ɁA�A���C�������g���w��
	int prevAlign = SetTextAlign(hdc, TA_TOP | TA_CENTER);
	int hw = width / 2;	// ��_�������ɂȂ邽�߁A�`��ʒu�𒲐�
	while (*str != _T('\0')) {
		TextOut(hdc, x * width + hw - ofsX, y, str++, 1);
		x++;
	}
	SetTextAlign(hdc, prevAlign);
}

int BinEdit::QuerySaveChanges(HWND hWnd) {
	int rc = 0;
	int answer = MessageBox(hWnd,
		_T("�t�@�C���T�C�Y���傫�����߁A�r���[��؂�ւ���K�v������܂��B\r\n")
		_T("�؂�ւ���O�ɁA�����܂ł̕ύX���t�@�C���ɕۑ����܂����H\r\n")
		_T("�ۑ����Ȃ��ꍇ�A�ύX�͎����܂��B"),
		szTitle, MB_YESNOCANCEL|MB_ICONEXCLAMATION);

	switch (answer) {
	case IDYES:
		fInfo.hfile.save();
		break;

	case IDNO:
		fInfo.hfile.resetDirty();
		break;

	case IDCANCEL:
		rc = -1;
		break;
	}
	return rc;
}

void BinEdit::ScrollUpDown(HWND hWnd, int lines, BOOL scrollCursor) {
	LONGLONG startLine = fInfo.startLine + lines;					// �}�C�i�X���l������̂ŕ����t��
	LONGLONG maxStartLine = fInfo.maxLine - fInfo.linesPerPage;
	if (maxStartLine < 0) {
		maxStartLine = 0;
	}

	if (startLine > maxStartLine) {
		startLine = maxStartLine;
	} else if (startLine < 0) {
		startLine = 0;
	}

	if (scrollCursor) {
		fInfo.cursorY += lines;
	}
	AdjustCursor();

	if (startLine != fInfo.startLine) {
		ULONGLONG firstPos = fInfo.startLine * 0x10;
		ULONGLONG lastPos = (fInfo.startLine + fInfo.linesPerPage) * 0x10 - 1;
		if (lastPos >= fInfo.hfile.getSize()) {
			lastPos = fInfo.hfile.getSize() - 1;
		}
		if (fInfo.hfile.get(firstPos) == ERR_DIRTY_VIEW
			|| fInfo.hfile.get(lastPos) == ERR_DIRTY_VIEW) {
				if (QuerySaveChanges(hWnd) != 0) {
					return;
				}
		}


		// �_���v�R�[�h���ĕ`��
		fInfo.startLine = startLine;
		InvalidateRect(hWnd, NULL, startLine == maxStartLine ? TRUE : FALSE);

		// �X�N���[���o�[���ĕ`��
		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		si.nPos = (int)fInfo.startLine;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
	}
}

void BinEdit::ReverseColor(HDC hdc) {
	COLORREF tmp = GetBkColor(hdc);
	SetBkColor(hdc, GetTextColor(hdc));
	SetTextColor(hdc, tmp);
}

// �J�[�\����L���͈͓��ɂȂ�悤�ɏC������
void BinEdit::AdjustCursor(void) {
	if (fInfo.cursorY < 0) {
		fInfo.cursorY = 0;
	} else if (fInfo.cursorY >= fInfo.linesPerPage) {
		fInfo.cursorY = fInfo.linesPerPage;
	}
	if (fInfo.startLine + fInfo.cursorY >= fInfo.maxLine) {
		fInfo.cursorY = (int)((fInfo.maxLine - 1) - fInfo.startLine);
	}

	if (fInfo.cursorX < 0) {
		fInfo.cursorX = 0;
	} else if (0x10 * 2 - 1 <= fInfo.cursorX) {
		fInfo.cursorX = 0x10 * 2 - 1;
	}
	if (fInfo.startLine + fInfo.cursorY == fInfo.maxLine - 1
		&& fInfo.cursorX > fInfo.lastXatLastLine * 2 -1) {
			fInfo.cursorX = fInfo.lastXatLastLine * 2 -1;
	}
}

void BinEdit::EnableCursor(HWND hWnd, BOOL enable) {
	fInfo.cursorEnabled = enable;

	RECT rect;
	rect.top = (fInfo.cursorY + DUMP_OFFSET_Y) * fInfo.cell_H;
	rect.bottom = rect.top + fInfo.cell_H;
	rect.left = (fInfo.cursorX + fInfo.cursorX / 2 + DUMP_OFFSET_X) * fInfo.cell_W;
	rect.right = rect.left + fInfo.cell_W;
	InvalidateRect(hWnd, &rect, FALSE);
	UpdateWindow(hWnd);
}

// �I�𒆂̃t�H���g�ɍ��킹�āA�Z���̕����v�Z����
int BinEdit::CalcCellWidth(HDC hdc) {
	// �g�p���镶���̒��ōő�̕����Z�����Ƃ���
	// �������A�v���|�[�V���i���t�H���g�̏ꍇ�͑S���̕����Œ��ׂ�Ɗԉ��т��Ă��܂��̂�
	// 16�i�_���v�Ɏg�������݂̂�Ώۂɂ���
	int maxWidth = 0;

	// �������̎擾�́ATrueType�t�H���g�������łȂ����ňقȂ邽��
	// �܂��t�H���g�̎�ނ𔻕�
	TEXTMETRIC tm;
	GetTextMetrics(hdc, &tm);

	if (tm.tmPitchAndFamily & TMPF_TRUETYPE) {
		// TrueType�t�H���g
		ABC abc[16];
		GetCharABCWidths(hdc, '0', '9', abc);
		GetCharABCWidths(hdc, 'A', 'F', &abc[10]);
		for (int i = 0; i < 16; i++) {
			int tmpW = abc[i].abcB;
			if (tmpW > maxWidth) {
				maxWidth = tmpW;
			}
		}
	} else {
		// ���X�^�[&�x�N�^�[�t�H���g�̏ꍇ
		int wid[16];
		GetCharWidth(hdc, '0', '9', wid);
		GetCharWidth(hdc, 'A', 'F', &wid[10]);
		for (int i = 0; i < 16; i++) {
			if (wid[i] > maxWidth) {
				maxWidth = wid[i];
			}
		}
	}
	return maxWidth;
}

void BinEdit::CreateMainFont(HWND hWnd) {
	HDC hdc = GetDC(hWnd);
	ZeroMemory(&fInfo.lf, sizeof(fInfo.lf));

	// �t�H���g�̍����́A���[�f�B���O�������܂߂Ȃ���`��10�|�C���g�Ƃ���
	// �_���t�H���g�̍����̒P�ʂ͘_���P�ʂȂ̂ŁA�ϊ�����K�v������
	fInfo.lf.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	_tcscpy_s(fInfo.lf.lfFaceName, _T("Times New Roman"));
	
	HFONT hFont = CreateFontIndirect(&fInfo.lf);
	if (hFont != NULL) {
		fInfo.hMainFont = hFont;

		HFONT hOrigFont = (HFONT)SelectObject(hdc, fInfo.hMainFont);
		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);
		SetCellSize(CalcCellWidth(hdc), tm.tmHeight);
		SelectObject(hdc, hOrigFont);
	}
	ReleaseDC(hWnd, hdc);
}

void BinEdit::GetCellRect(RECT* pRect, int x, int y) {
	if (pRect == NULL) {
		return;
	}

	pRect->top = (y + DUMP_OFFSET_Y) * fInfo.cell_H;
	pRect->bottom = pRect->top + fInfo.cell_H;
	pRect->left = (x + x/2 + DUMP_OFFSET_X) * fInfo.cell_W;
	pRect->right = pRect->left + fInfo.cell_W;
}

void BinEdit::SetCellSize(int width, int height) {
	fInfo.cell_W = width;
	fInfo.cell_H = height;
}
