#pragma once

#include <Windows.h>

// �G���[�E�R�[�h�̒�`
#define SUCCESS					0
#define ERR_INVALID_ARGS		-1		// �p�����[�^���s��
#define ERR_BEYOND_FILE			-2		// �t�@�C���E�T�C�Y�𒴂����ʒu�w��
#define ERR_OUT_OF_VIEW			-3		// �r���[�O�̈ʒu�w��
#define ERR_DIRTY_VIEW			-4		// �r���[���ҏW����Ă���
#define ERR_CANNOT_MAP_VIEW		-5		// �r���[���}�b�v�ł��Ȃ�
#define ERR_FILE_NOT_OPENED		-6		// �t�@�C�����I�[�v������Ă��Ȃ�
#define ERR_CANNOT_OPEN_FILE	-7		// �t�@�C�����I�[�v���ł��Ȃ�
#define ERR_ZERO_SIZE_FILE		-8		// �t�@�C���E�T�C�Y��0
#define ERR_CANNOT_MAP_FILE		-9		// �t�@�C�����}�b�v�ł��Ȃ�
#define ERR_FILE_ALREADY_OPENED	-10		// �t�@�C���͂��łɃI�[�v������Ă���
#define ERR_UNKNOWN				-99		// ���̑��̃G���[

class HugeFile
{
public:
	HugeFile(void);
	~HugeFile(void);

	int open(LPCTSTR fname);
	int close(BOOL toSave = TRUE);
	int save(void);
	
	BOOL isDirty(void) { return m_dirty; }
	void resetDirty(void){ m_dirty = FALSE; }

	// �t�@�C���̎w��ʒu�Ƀf�[�^���Z�b�g
	int set(ULONGLONG index, int data);

	// �t�@�C���̎w��ʒu�̃f�[�^���擾
	int get(ULONGLONG index);

	// ���[�h����Ă���f�[�^�̐擪�̃t�@�C����̃I�t�Z�b�g
	ULONGLONG getBaseIndex(void) { return m_viewOffset; }

	// �t�@�C���T�C�Y���擾
	ULONGLONG getSize(void) { return m_fileSize; }

	// �t�@�C�������[�h�����ǂ���
	BOOL isLoaded(void) { return m_file != INVALID_HANDLE_VALUE; }

private:
	HANDLE m_file;
	HANDLE m_mappedFile;	// �t�@�C���}�b�s���O

	// �t�@�C���̏��
	ULONGLONG m_fileSize;

	// �r���[�̏��
	BYTE* m_pView;			// �r���[�̊J�n�A�h���X
	SIZE_T m_viewSize;		// �r���[�̃T�C�Y
	SIZE_T m_viewUnit;		// �r���[�̐؂�ւ��P��
	ULONGLONG m_viewOffset;	// �r���[�ɑΉ�����t�@�C���̃I�t�Z�b�g
	BOOL m_dirty;

	// �V�X�e���̃p�����[�^
	DWORD m_allocUnit;		// ���z���������蓖�ĒP��
	DWORD m_pageSize;		// �y�[�W�T�C�Y

	// ���ʂ̏��������[�`��
	void initialize(void);

	// �r���[�ʒu�̕ύX
	int changeView(ULONGLONG index);

	// �ł��邾���傫�ȃr���[���}�b�v����
	int mapMaxView(void);
};

