#include "HugeFile.h"

HugeFile::HugeFile(void)
{
	initialize();

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	m_allocUnit = sysInfo.dwAllocationGranularity;
	m_pageSize = sysInfo.dwPageSize;
}


HugeFile::~HugeFile(void)
{
}

int HugeFile::open(LPCTSTR fname) {
	int rc = SUCCESS;

	if (m_file != INVALID_HANDLE_VALUE) {
		return ERR_FILE_ALREADY_OPENED;
	}

	DWORD access = GENERIC_READ | GENERIC_WRITE;
	m_file = CreateFile(fname, access, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (m_file != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fsize;
		GetFileSizeEx(m_file, &fsize);
		m_fileSize = fsize.QuadPart;

		if (m_fileSize > 0) {	// ファイルサイズが0ではだめ
			m_mappedFile = CreateFileMapping(m_file, NULL, PAGE_READWRITE, 0, 0, NULL);

			if (m_mappedFile != NULL) {
				rc = mapMaxView();
			} else {
				return ERR_CANNOT_MAP_FILE;
			}

		} else {
			return ERR_ZERO_SIZE_FILE;
		}

	} else {
		return ERR_CANNOT_OPEN_FILE;
	}

	if (m_pView == NULL) {
		close();
	}
	return rc;
}

int HugeFile::close(BOOL toSave) {
	if (m_file == INVALID_HANDLE_VALUE) {
		return ERR_FILE_NOT_OPENED;
	}
	if (toSave) {
		save();
	}
	if (m_mappedFile != INVALID_HANDLE_VALUE) {
		UnmapViewOfFile(m_pView);
	}
	CloseHandle(m_mappedFile);
	CloseHandle(m_file);
	initialize();
	return SUCCESS;
}

int HugeFile::save(void) {
	int rc = SUCCESS;
	if (m_file == INVALID_HANDLE_VALUE) {
		return ERR_FILE_NOT_OPENED;
	}

	if (m_dirty) {
		// ビューの先頭から保護属性を調べていく
		BYTE* pScan = m_pView;
		MEMORY_BASIC_INFORMATION mbi;
		while (pScan < m_pView + m_viewSize) {
			if (!VirtualQuery(pScan, &mbi, sizeof(mbi))) {
				// エラーは起きないはず
				break;
			}

			// 書きこまれたページが見つかった場合
			if (mbi.Protect == PAGE_READWRITE) {
				ULONGLONG fileOffset = m_viewOffset + (pScan - m_pView);
				int unitCount = (int)(fileOffset/m_allocUnit);
				ULONGLONG fileBase = (ULONGLONG)m_allocUnit * unitCount;
				fileOffset -= fileBase;
				SIZE_T copySize = min(m_pageSize, m_viewSize);

				// 上で計算した位置に、ファイル書き込み用のビューを別途作成
				BYTE* pWrite = (BYTE*)MapViewOfFile(m_mappedFile, FILE_MAP_WRITE, (DWORD)(fileBase >> 32), (DWORD)(fileBase & 0xFFFFFFFF), copySize);
				if (pWrite != NULL) {
					// 編集用のビューから書き込み用のビューへ
					// 1ページ分データをコピー
					memcpy(pWrite + fileOffset, pScan, copySize);
					UnmapViewOfFile(pWrite);	// 書き込みビューを解除
				} else {
					TCHAR szBuffer[256];
					wsprintf(szBuffer, TEXT("%d\n"), GetLastError());
					OutputDebugString(szBuffer);
				}
				pScan += copySize;	// 次のページをチェック

			} else {
				// 保護属性が変化する次の境界までジャンプ
				pScan = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
			}
		}
		resetDirty();
		// 保存が終わったので、ビューの読み込み直しを行う
		changeView(m_viewOffset);
	}
	return rc;
}
	
// ファイルの指定位置にデータをセット
int HugeFile::set(ULONGLONG index, int data) {
	int rc = SUCCESS;
	if (m_file == INVALID_HANDLE_VALUE) {
		return ERR_FILE_NOT_OPENED;
	}
	if (data < 0 || 255 < data) {
		return ERR_INVALID_ARGS;
	}

	// インデックスが現在のビューに含まれるとき
	if (m_viewOffset <= index && index < m_viewOffset + m_viewSize) {
		SIZE_T viewIndex = (SIZE_T)(index - m_viewOffset);	// 現在のビューからのインデックス
		int prevData = m_pView[viewIndex];
		if (prevData != data) {
			m_pView[viewIndex] = (BYTE)data;
			m_dirty = TRUE;
		}

	} else if (m_dirty) {
		// 編集フラグがセットされており、ビューも変更する必要がある
		rc = ERR_DIRTY_VIEW;
	} else {
		if ((rc = changeView(index)) == SUCCESS) {
			rc = set(index, data);
		}
	}
	return rc;
}

// ファイルの指定位置のデータを取得
int HugeFile::get(ULONGLONG index) {
	if (m_file == INVALID_HANDLE_VALUE) {
		return ERR_FILE_NOT_OPENED;
	}
	int data = ERR_UNKNOWN;

	if (m_viewOffset <= index && index < m_viewOffset + m_viewSize) {
		data = m_pView[(SIZE_T)(index - m_viewOffset)];

	} else if (m_dirty) {
		data = ERR_DIRTY_VIEW;

	} else {
		int rc = SUCCESS;
		if ((rc = changeView(index)) == SUCCESS) {
			data = get(index);
		} else {
			data = rc;
		}
	}
	return data;
}



//////////////////////////////////////////////////////////////////

void HugeFile::initialize(void) {
	m_file = INVALID_HANDLE_VALUE;
	m_mappedFile = INVALID_HANDLE_VALUE;

	m_fileSize = 0;

	m_pView = NULL;
	m_viewSize = 0;
	m_viewUnit = 0;
	m_viewOffset = 0;
	m_dirty = FALSE;
}

// ビュー位置の変更
int HugeFile::changeView(ULONGLONG index) {
	if (index >= m_fileSize) {
		return ERR_BEYOND_FILE;
	}

	// インデックスに対応する、ビューのオフセットを計算
	int unitCount = (int)(index/m_viewUnit);
	ULONGLONG newOffset = m_viewUnit * unitCount;

	UnmapViewOfFile(m_pView);
	m_viewOffset = newOffset;

	// ビューのサイズがファイルサイズを超えないように調整
	if (m_viewSize + newOffset > m_fileSize) {
		m_viewSize = (SIZE_T)(m_fileSize - m_viewOffset);
	}

	// 新しい位置でビューをマップ
	// 解除したばかりのビューと同じサイズで行けるはず
	m_pView = (BYTE*)MapViewOfFile(m_mappedFile, FILE_MAP_COPY, (DWORD)(newOffset >> 32), (DWORD)(newOffset & 0xFFFFFFFF), m_viewSize);

	m_dirty = FALSE;
	return (m_pView != NULL) ? SUCCESS : ERR_CANNOT_MAP_VIEW;
}

// できるだけ大きなビューをマップする
int HugeFile::mapMaxView(void) {
	SIZE_T viewSize = 0;
	
	// ファイル全体でトライ
	BYTE* ptr = (BYTE*)MapViewOfFile(m_mappedFile, FILE_MAP_COPY, 0, 0, 0);
	if (ptr == NULL) {
		// ファイル全体は収まらなかったのでより小さいサイズで試す
		if (sizeof(SIZE_T) > 4) {
			viewSize = (SIZE_T)m_fileSize;

		} else {
			if (m_fileSize > 0x40000000) {
				viewSize = 0x40000000;
			} else {
				viewSize = (SIZE_T)m_fileSize;
			}
		}

		// 端数があると計算が面倒なので、
		// ファイルサイズで1が立っている最上位ビットだけ残し、それより下位のビットは0にする
		SIZE_T mask = (SIZE_T)1 << (sizeof(SIZE_T)*8 - 1);
		while (mask > 0) {
			if ((viewSize & mask) != 0) {
				viewSize = mask;
				break;
			}
			mask >>= 1;
		}

		while (ptr == NULL && viewSize > 0) {
			ptr = (BYTE*)MapViewOfFile(m_mappedFile, FILE_MAP_COPY, 0, 0, viewSize);
			if (ptr == NULL) {
				viewSize >>= 1;
			}
		}
		m_viewUnit = viewSize;
		m_viewSize = viewSize;

	} else {
		m_viewSize = (SIZE_T)m_fileSize;
		m_viewUnit = m_viewSize;
	}

	m_pView = ptr;
	return (ptr != NULL) ? SUCCESS : ERR_CANNOT_MAP_VIEW;
}
