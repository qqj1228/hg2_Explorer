#pragma once

#include "resource.h"

#define MAX_LOADSTRING 100
#define FILENUM 0x602

typedef struct tagFILEINFO
{
	char szName[8];
	char szType[8];
	char szOffset[11];
	char szSize[11];
	DWORD dwOffset;
	DWORD dwSize;
}FILEINFO;

INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
int ConfirmType(LPCSTR szType);