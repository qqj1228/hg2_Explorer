// hg2_Explorer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "hg2_Explorer.h"

// Global Variables:
HINSTANCE g_hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
FILEINFO fi[FILENUM];
DWORD dwFilePos;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_HG2_EXPLORER, szWindowClass, MAX_LOADSTRING);

	g_hInst=hInstance;
	DialogBox(g_hInst,(LPCSTR)IDD_DIALOG_MAIN,NULL,(DLGPROC)DlgProc);
	ExitProcess(NULL);
	return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	int i,wmId, wmEvent, iCharCount;
	char szText[256];
	FILE *fpIndex, *fpData, *fpOut, *fpIn;
	static HWND hWndListView;
	HMENU hPopupMenu;
	POINT pt;
	static int index, lvIndex;
	BYTE *lpByBuf;
	LVCOLUMN lvc;
	LVITEM lvI;
	NMLVDISPINFO *plvdi;
	LPNMITEMACTIVATE lpnmitem;
	OPENFILENAME ofn;
	char szFile[MAX_PATH], szFileName[MAX_PATH], szDataPath[MAX_PATH], szOutPath[MAX_PATH], szInPath[MAX_PATH];
	static char szCurrentPath[MAX_PATH];
	int nName;
	char szOffset[256];

	switch (message)
	{
	case WM_INITDIALOG:
		InitCommonControls();

		GetCurrentDirectory(MAX_PATH,(LPTSTR)szCurrentPath);
		//读取索引信息
		if (fopen_s(&fpIndex, "index.bin", "rb")!=NULL)
		{
			MessageBox(hDlg, "Can't open index.bin!", szTitle, MB_OK | MB_ICONERROR);
			break;
		}
		if (fopen_s(&fpData, "DATA.DAT", "rb")!=NULL)
		{
			MessageBox(hDlg, "Can't open DATA.DAT!", szTitle, MB_OK | MB_ICONERROR);
			break;
		}

		for(i=0; i<FILENUM; i++)
		{
			fread(&fi[i].dwOffset, 4, 1, fpIndex);
			fread(&fi[i].dwSize, 4, 1, fpIndex);
			sprintf_s(fi[i].szName, 8, "%03X.bin", i);
			fseek(fpData, fi[i].dwOffset, 0);
			fread(&fi[i].szType, 8, 1, fpData);
			iCharCount=ConfirmType(fi[i].szType);
			if(iCharCount>=3 && iCharCount<=4)
				fi[i].szType[iCharCount]='\0';
			else if(iCharCount>4 && iCharCount<7)
				fi[i].szType[4]='\0';
			else if(7==iCharCount)
				fi[i].szType[iCharCount]='\0';
			else
				sprintf_s(fi[i].szType, 8, "RAW");
			sprintf_s(fi[i].szOffset, 11, "0x%08X", fi[i].dwOffset);
			sprintf_s(fi[i].szSize, 11, "0x%08X", fi[i].dwSize);
		}
		fclose(fpIndex);
		fclose(fpData);
		
		//设置ListView控件
		hWndListView=GetDlgItem(hDlg, IDC_LISTVIEW);
		ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_BORDERSELECT | LVS_EX_FULLROWSELECT);
		//ListView_SetItemCount(hWndListView, FILENUM);
		ZeroMemory(&lvc, sizeof(LVCOLUMN));
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		for (i = 0; i < 4; i++)
		{ 
			lvc.iSubItem = i;
			lvc.pszText = szText;
			lvc.cx = 100;     // width of column in pixels
			lvc.fmt = LVCFMT_LEFT;  // left-aligned column
			LoadString(g_hInst, IDS_COLUMN1 + i, szText, sizeof(szText)/sizeof(szText[0]));
			if (ListView_InsertColumn(hWndListView, i, &lvc) == -1) 
				break;
		}
		//ListView_DeleteAllItems(hWndListView);
		lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
		lvI.state = 0;
		lvI.stateMask = 0;
		for (i = 0; i < FILENUM; i++)
		{
			lvI.iItem = i;
			lvI.iSubItem = 0;
			lvI.lParam = (LPARAM)i;// &fi[i];
			lvI.pszText = LPSTR_TEXTCALLBACK; // sends an LVN_GETDISP message.
			if(ListView_InsertItem(hWndListView, &lvI) == -1)
				break;
		}
		return (INT_PTR)TRUE;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code)
		{
		case LVN_GETDISPINFO:
			plvdi=(NMLVDISPINFO*)lParam;
			switch (plvdi->item.iSubItem)
			{
			case 0:
				plvdi->item.pszText = fi[plvdi->item.lParam].szName;
				break;  
			case 1:
				plvdi->item.pszText = fi[plvdi->item.lParam].szType;
				break;
			case 2:
				plvdi->item.pszText = fi[plvdi->item.lParam].szOffset;
				break;
			case 3:
				plvdi->item.pszText = fi[plvdi->item.lParam].szSize;
				break;
			}
			break;
		case NM_RCLICK:
			lpnmitem = (LPNMITEMACTIVATE) lParam;
			lvIndex=lpnmitem->iItem;
			if(lvIndex==-1)
				break;
			ZeroMemory(&lvI, sizeof(LVITEM));
			lvI.mask=LVIF_PARAM;
			lvI.iItem=lvIndex;
			if(SendMessage(hWndListView, LVM_GETITEM, 0, (LPARAM) (LPLVITEM)&lvI))
				index=lvI.lParam;
			else
				break;
			hPopupMenu = GetSubMenu(LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_POPUPMENU)),0);
			GetCursorPos(&pt);
			TrackPopupMenuEx(hPopupMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, pt.x, pt.y, hDlg, NULL);
			break;
		}
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_EXPORT:
			sprintf_s(szDataPath, MAX_PATH, "%s\\DATA.DAT", szCurrentPath);
			if (fopen_s(&fpData, szDataPath, "rb")!=NULL)
			{
				MessageBox(hDlg, "Can't open DATA.DAT!", szTitle, MB_OK | MB_ICONERROR);
				break;
			}
			sprintf_s(szOutPath, MAX_PATH, "%s\\%s", szCurrentPath, fi[index].szName);
			if (fopen_s(&fpOut, szOutPath, "wb")!=NULL)
			{
				MessageBox(hDlg, "Can't create exporting file!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpData);
				break;
			}
			lpByBuf=(BYTE*)malloc(fi[index].dwSize*sizeof(BYTE));
			ZeroMemory(lpByBuf, (fi[index].dwSize)*sizeof(BYTE));
			fseek(fpData, fi[index].dwOffset, 0);
			fread(lpByBuf, 1, fi[index].dwSize, fpData);
			if(fi[index].dwSize!=fwrite(lpByBuf, 1, fi[index].dwSize, fpOut))
			{
				MessageBox(hDlg, "Exporting file occur error!", szTitle, MB_OK | MB_ICONERROR);
				free(lpByBuf);
				lpByBuf=NULL;
				fclose(fpOut);
				fclose(fpData);
				break;
			}
			free(lpByBuf);
			lpByBuf=NULL;
			fclose(fpOut);
			fclose(fpData);
			MessageBox(hDlg, "Exporting file succeed!", szTitle, MB_OK | MB_ICONINFORMATION);
			break;
		case IDM_IMPORT:
			ZeroMemory(szFile, sizeof(szFile[MAX_PATH]));
			ZeroMemory(szFileName, sizeof(szFileName[MAX_PATH]));
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "BIN File(*.bin)\0*.bin\0All Files(*.*)\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = szFileName;
			ofn.nMaxFileTitle = sizeof(szFileName);
			ofn.lpstrInitialDir = NULL;
			//ofn.lpstrDefExt = "vlg";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

			if (GetOpenFileName(&ofn)!=TRUE) 
				break;

			sprintf_s(szDataPath, MAX_PATH, "%s\\DATA.DAT", szCurrentPath);
			if (fopen_s(&fpData, szDataPath, "rb+")!=NULL)
			{
				MessageBox(hDlg, "Can't open DATA.DAT!", szTitle, MB_OK | MB_ICONERROR);
				break;
			}
			if (fopen_s(&fpIn, szFile, "rb")!=NULL)
			{
				MessageBox(hDlg, "Can't open Import File!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpData);
				break;
			}
			fseek(fpIn, 0L, 2);
			dwFilePos=ftell(fpIn);
			if (fi[index].dwSize!=dwFilePos)
			{
				MessageBox(hDlg, "The file to import doesn't match the selected file!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpIn);
				fclose(fpData);
				break;
			}
			lpByBuf=(BYTE*)malloc(fi[index].dwSize*sizeof(BYTE));
			ZeroMemory(lpByBuf, (fi[index].dwSize)*sizeof(BYTE));
			rewind(fpIn);
			dwFilePos=ftell(fpIn);
			fread(lpByBuf, 1, fi[index].dwSize, fpIn);
			fseek(fpData, fi[index].dwOffset, 0);
			dwFilePos=ftell(fpData);
			if(fi[index].dwSize!=fwrite(lpByBuf, 1, fi[index].dwSize, fpData))
			{
				MessageBox(hDlg, "Importing file occur error!", szTitle, MB_OK | MB_ICONERROR);
				free(lpByBuf);
				lpByBuf=NULL;
				fclose(fpIn);
				fclose(fpData);
				break;
			}
			free(lpByBuf);
			lpByBuf=NULL;
			fclose(fpIn);
			fclose(fpData);
			MessageBox(hDlg, "Importing file succeed!", szTitle, MB_OK | MB_ICONINFORMATION);
			break;
		case IDC_BUTTON1:
			sprintf_s(szInPath, MAX_PATH, "%s\\in.txt", szCurrentPath);
			if (fopen_s(&fpIn, szInPath, "rt")!=NULL)
			{
				MessageBox(hDlg, "Can't open in.txt!", szTitle, MB_OK | MB_ICONERROR);
				break;
			}
			sprintf_s(szOutPath, MAX_PATH, "%s\\out.txt", szCurrentPath);
			if (fopen_s(&fpOut, szOutPath, "wt")!=NULL)
			{
				MessageBox(hDlg, "Can't open out.txt!", szTitle, MB_OK | MB_ICONERROR);
				fclose(fpIn);
				break;
			}
			while(1)
			{
				if(EOF==fscanf_s(fpIn, "%X", &nName))
					break;
				memset(&lvI, 0, sizeof(LVITEM));
				lvI.mask=LVIF_TEXT;
				lvI.iItem=nName;
				lvI.iSubItem=2;
				lvI.pszText=szOffset;
				lvI.cchTextMax=256;
				SendMessage(hWndListView, LVM_GETITEM, (WPARAM)0, (LPARAM)&lvI);
				fprintf_s(fpOut, "%s,\n", szOffset);
			}
			fclose(fpIn);
			fclose(fpOut);
			MessageBox(hDlg, "\"Output Offset\" succeed!", szTitle, MB_OK | MB_ICONINFORMATION);
			break;
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			break;
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
	}
	return (INT_PTR)FALSE;
}

int ConfirmType(LPCSTR szType)
{
	int i, count=0;
	char szBuf[8];

	strcpy_s(szBuf, 8, szType);
	for (i=0; i<7; i++)
	{
		if ((szBuf[i]<'0'||szBuf[i]>'9')&&(szBuf[i]<'A'||szBuf[i]>'Z')&&(szBuf[i]<'a'||szBuf[i]>'z'))
			break;
		count++;
	}
	return count;
}