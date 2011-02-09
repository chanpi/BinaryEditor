#pragma once

#include <Windows.h>

// エラー・コードの定義
#define SUCCESS					0
#define ERR_INVALID_ARGS		-1		// パラメータが不正
#define ERR_BEYOND_FILE			-2		// ファイル・サイズを超えた位置指定
#define ERR_OUT_OF_VIEW			-3		// ビュー外の位置指定
#define ERR_DIRTY_VIEW			-4		// ビューが編集されている
#define ERR_CANNOT_MAP_VIEW		-5		// ビューをマップできない
#define ERR_FILE_NOT_OPENED		-6		// ファイルがオープンされていない
#define ERR_CANNOT_OPEN_FILE	-7		// ファイルがオープンできない
#define ERR_ZERO_SIZE_FILE		-8		// ファイル・サイズが0
#define ERR_CANNOT_MAP_FILE		-9		// ファイルをマップできない
#define ERR_FILE_ALREADY_OPENED	-10		// ファイルはすでにオープンされている
#define ERR_UNKNOWN				-99		// その他のエラー

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

	// ファイルの指定位置にデータをセット
	int set(ULONGLONG index, int data);

	// ファイルの指定位置のデータを取得
	int get(ULONGLONG index);

	// ロードされているデータの先頭のファイル上のオフセット
	ULONGLONG getBaseIndex(void) { return m_viewOffset; }

	// ファイルサイズを取得
	ULONGLONG getSize(void) { return m_fileSize; }

	// ファイルがロード中かどうか
	BOOL isLoaded(void) { return m_file != INVALID_HANDLE_VALUE; }

private:
	HANDLE m_file;
	HANDLE m_mappedFile;	// ファイルマッピング

	// ファイルの情報
	ULONGLONG m_fileSize;

	// ビューの情報
	BYTE* m_pView;			// ビューの開始アドレス
	SIZE_T m_viewSize;		// ビューのサイズ
	SIZE_T m_viewUnit;		// ビューの切り替え単位
	ULONGLONG m_viewOffset;	// ビューに対応するファイルのオフセット
	BOOL m_dirty;

	// システムのパラメータ
	DWORD m_allocUnit;		// 仮想メモリ割り当て単位
	DWORD m_pageSize;		// ページサイズ

	// 共通の初期化ルーチン
	void initialize(void);

	// ビュー位置の変更
	int changeView(ULONGLONG index);

	// できるだけ大きなビューをマップする
	int mapMaxView(void);
};

