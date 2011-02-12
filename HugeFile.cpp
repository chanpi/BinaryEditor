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

		if (m_fileSize > 0) {	// �t�@�C���T�C�Y��0�ł͂���
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
		// �r���[�̐擪����ی쑮���𒲂ׂĂ���
		BYTE* pScan = m_pView;
		MEMORY_BASIC_INFORMATION mbi;
		while (pScan < m_pView + m_viewSize) {
			if (!VirtualQuery(pScan, &mbi, sizeof(mbi))) {
				// �G���[�͋N���Ȃ��͂�
				break;
			}

			// �������܂ꂽ�y�[�W�����������ꍇ
			if (mbi.Protect == PAGE_READWRITE) {
				ULONGLONG fileOffset = m_viewOffset + (pScan - m_pView);
				int unitCount = (int)(fileOffset/m_allocUnit);
				ULONGLONG fileBase = (ULONGLONG)m_allocUnit * unitCount;
				fileOffset -= fileBase;
				SIZE_T copySize = min(m_pageSize, m_viewSize);

				// ��Ōv�Z�����ʒu�ɁA�t�@�C���������ݗp�̃r���[��ʓr�쐬
				BYTE* pWrite = (BYTE*)MapViewOfFile(m_mappedFile, FILE_MAP_WRITE, (DWORD)(fileBase >> 32), (DWORD)(fileBase & 0xFFFFFFFF), copySize);
				if (pWrite != NULL) {
					// �ҏW�p�̃r���[���珑�����ݗp�̃r���[��
					// 1�y�[�W���f�[�^���R�s�[
					memcpy(pWrite + fileOffset, pScan, copySize);
					UnmapViewOfFile(pWrite);	// �������݃r���[������
				} else {
					TCHAR szBuffer[256];
					wsprintf(szBuffer, TEXT("%d\n"), GetLastError());
					OutputDebugString(szBuffer);
				}
				pScan += copySize;	// ���̃y�[�W���`�F�b�N

			} else {
				// �ی쑮�����ω����鎟�̋��E�܂ŃW�����v
				pScan = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
			}
		}
		resetDirty();
		// �ۑ����I������̂ŁA�r���[�̓ǂݍ��ݒ������s��
		changeView(m_viewOffset);
	}
	return rc;
}
	
// �t�@�C���̎w��ʒu�Ƀf�[�^���Z�b�g
int HugeFile::set(ULONGLONG index, int data) {
	int rc = SUCCESS;
	if (m_file == INVALID_HANDLE_VALUE) {
		return ERR_FILE_NOT_OPENED;
	}
	if (data < 0 || 255 < data) {
		return ERR_INVALID_ARGS;
	}

	// �C���f�b�N�X�����݂̃r���[�Ɋ܂܂��Ƃ�
	if (m_viewOffset <= index && index < m_viewOffset + m_viewSize) {
		SIZE_T viewIndex = (SIZE_T)(index - m_viewOffset);	// ���݂̃r���[����̃C���f�b�N�X
		int prevData = m_pView[viewIndex];
		if (prevData != data) {
			m_pView[viewIndex] = (BYTE)data;
			m_dirty = TRUE;
		}

	} else if (m_dirty) {
		// �ҏW�t���O���Z�b�g����Ă���A�r���[���ύX����K�v������
		rc = ERR_DIRTY_VIEW;
	} else {
		if ((rc = changeView(index)) == SUCCESS) {
			rc = set(index, data);
		}
	}
	return rc;
}

// �t�@�C���̎w��ʒu�̃f�[�^���擾
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

// �r���[�ʒu�̕ύX
int HugeFile::changeView(ULONGLONG index) {
	if (index >= m_fileSize) {
		return ERR_BEYOND_FILE;
	}

	// �C���f�b�N�X�ɑΉ�����A�r���[�̃I�t�Z�b�g���v�Z
	int unitCount = (int)(index/m_viewUnit);
	ULONGLONG newOffset = m_viewUnit * unitCount;

	UnmapViewOfFile(m_pView);
	m_viewOffset = newOffset;

	// �r���[�̃T�C�Y���t�@�C���T�C�Y�𒴂��Ȃ��悤�ɒ���
	if (m_viewSize + newOffset > m_fileSize) {
		m_viewSize = (SIZE_T)(m_fileSize - m_viewOffset);
	}

	// �V�����ʒu�Ńr���[���}�b�v
	// ���������΂���̃r���[�Ɠ����T�C�Y�ōs����͂�
	m_pView = (BYTE*)MapViewOfFile(m_mappedFile, FILE_MAP_COPY, (DWORD)(newOffset >> 32), (DWORD)(newOffset & 0xFFFFFFFF), m_viewSize);

	m_dirty = FALSE;
	return (m_pView != NULL) ? SUCCESS : ERR_CANNOT_MAP_VIEW;
}

// �ł��邾���傫�ȃr���[���}�b�v����
int HugeFile::mapMaxView(void) {
	SIZE_T viewSize = 0;
	
	// �t�@�C���S�̂Ńg���C
	BYTE* ptr = (BYTE*)MapViewOfFile(m_mappedFile, FILE_MAP_COPY, 0, 0, 0);
	if (ptr == NULL) {
		// �t�@�C���S�͎̂��܂�Ȃ������̂ł�菬�����T�C�Y�Ŏ���
		if (sizeof(SIZE_T) > 4) {
			viewSize = (SIZE_T)m_fileSize;

		} else {
			if (m_fileSize > 0x40000000) {
				viewSize = 0x40000000;
			} else {
				viewSize = (SIZE_T)m_fileSize;
			}
		}

		// �[��������ƌv�Z���ʓ|�Ȃ̂ŁA
		// �t�@�C���T�C�Y��1�������Ă���ŏ�ʃr�b�g�����c���A�����艺�ʂ̃r�b�g��0�ɂ���
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
