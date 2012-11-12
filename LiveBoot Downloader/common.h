/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:18
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader\common.h
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader
	file base:	common
	file ext:	h
	author:		tanyc
	
	purpose:	
*********************************************************************/
#pragma once
#include "stdafx.h"
#include <atlimage.h>
#include <shellapi.h>
#include "../dll_downloader/MD5.h"
#include "PidInfo.h"
#include "../../Common/util.h"

extern CPIDInfos g_pid_info;
//---------------------------------------------------------------------------------------//
inline bool w2a(LPCWSTR lpcwszStr, LPSTR lpszStr, DWORD dwSize)
{
	if((NULL == lpcwszStr) || (NULL == lpszStr) || (dwSize <= 0))
	{
		return false;
	}


	DWORD dwMinSize = 0;
	dwMinSize = WideCharToMultiByte(CP_ACP, 0, lpcwszStr, -1, NULL, 0, NULL, NULL);
	if(0 == dwMinSize)
	{
		return false;
	}
	else
	{
		if(dwSize < dwMinSize)
		{
			return false;
		}

		if(0 == WideCharToMultiByte(CP_ACP, 0, lpcwszStr, -1, lpszStr, dwSize, NULL, NULL))
		{
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------------------------------------//
inline bool a2w(LPCSTR lpszStr, LPWSTR lpcwszStr, DWORD dwSize) 
{ 
	if((NULL == lpcwszStr) || (NULL == lpszStr) || (dwSize <= 0))
	{
		return false;
	}

	DWORD dwMinSize = 0;
	dwMinSize = MultiByteToWideChar(CP_ACP, 0, lpszStr, strlen(lpszStr) + 1, NULL, 0);
	if(0 == dwMinSize)
	{
		return false;
	}
	else
	{
		if(dwSize < dwMinSize)
		{
			return false;
		}
		if(0 == MultiByteToWideChar (CP_ACP, 0, lpszStr, strlen(lpszStr) + 1, lpcwszStr, dwMinSize))
		{
			return false;
		}
	}
	return true;
}

inline HBITMAP GetSubBitmap(HDC ClientDC, HBITMAP bmp, RECT rect)
{
	HBITMAP hSourceHbitmap = (HBITMAP)bmp;
	HDC sourceDC = ::CreateCompatibleDC(NULL);  
	HDC destDC = ::CreateCompatibleDC(NULL);  
  
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	BITMAP bm = {0};
	::GetObject(hSourceHbitmap, sizeof(bm), &bm);  
	HBITMAP hbmResult = ::CreateCompatibleBitmap(ClientDC, width, height);   

	HBITMAP hbmOldSource = (HBITMAP)::SelectObject(sourceDC, hSourceHbitmap);   
	HBITMAP hbmOldDest  = (HBITMAP)::SelectObject(destDC, hbmResult );  
	::BitBlt(destDC, 0, 0, width, height, sourceDC, rect.left, rect.top, SRCCOPY);

	::SelectObject(sourceDC, hbmOldSource);   
	::SelectObject(destDC, hbmOldDest);
	::DeleteDC(sourceDC);
	::DeleteDC(destDC);
	return hbmResult;
}

inline BOOL IsFileExist(CString szFullFileName)
{
	BOOL bExist = FALSE;
	if(!szFullFileName.IsEmpty())
	{
		HANDLE hFile = ::CreateFile(szFullFileName,			// file to open
			GENERIC_READ,									// open for read
			FILE_SHARE_READ,								// share for reading
			NULL,											// default security
			OPEN_EXISTING,									// existing file only
			FILE_ATTRIBUTE_NORMAL,							// normal file
			NULL);											// no attr. template
		if (hFile != INVALID_HANDLE_VALUE) 
		{
			CloseHandle(hFile);
			bExist = TRUE;			
		}
	}
	return bExist;
}

inline HBITMAP LoadBitmapFromFile(CString szFile)
{
	if(szFile.IsEmpty() || !IsFileExist(szFile))
	{
		return NULL;
	}

	CImage image;
	if(FAILED(image.Load(szFile)))
	{
		return NULL;
	}
	return (HBITMAP)image;
}


inline HICON LoadIconFromFile(CString szFile)
{
	if(szFile.IsEmpty() || !IsFileExist(szFile))
	{
		return NULL;
	}

	return (HICON)LoadImage(NULL, szFile, IMAGE_ICON, 0, 0 ,LR_LOADFROMFILE);
}

inline Image* ImageFromIDResource(HINSTANCE hInst, UINT nID, LPCTSTR sTR)
{
	HRSRC hRsrc = ::FindResource (hInst, MAKEINTRESOURCE(nID), sTR);
	if (!hRsrc)
	{
		return NULL;
	}

	// load resource into memory
	DWORD len = ::SizeofResource(hInst, hRsrc);
	BYTE* lpRsrc = (BYTE*)::LoadResource(hInst, hRsrc);
	if (!lpRsrc)
	{
		return NULL;
	}

	// Allocate global memory on which to create stream
	HGLOBAL m_hMem = ::GlobalAlloc(GMEM_FIXED, len);
	BYTE* pmem = (BYTE*)::GlobalLock(m_hMem);
	memcpy(pmem,lpRsrc,len);
	IStream* pstm;
	::CreateStreamOnHGlobal(m_hMem,FALSE,&pstm);

	// load from stream
	Image *pImg = Gdiplus::Image::FromStream(pstm);
	Status stat = pImg->GetLastStatus();  
	if(stat != Ok)
	{
		if(pImg)
		{
			delete pImg;
			pImg = NULL;
		}
	}

	// free/release stuff
	::GlobalUnlock(m_hMem);
	pstm->Release();
	::FreeResource(lpRsrc);
	return pImg;
}

inline CString ShowBrowseCommonDialog(HWND hWnd, LPCTSTR pszTitle)
{
	CString szPath;
	BROWSEINFO m_bi = {0};
	wchar_t m_szDisplay[MAX_PATH] = {0};
	wchar_t m_szPath[MAX_PATH] = {0};
	memset(&m_bi, 0, sizeof(m_bi));
	m_bi.hwndOwner = NULL;
	m_bi.pidlRoot = NULL;  
	m_bi.pszDisplayName = m_szDisplay;
	m_bi.lpszTitle = NULL;  
	m_bi.ulFlags = BIF_RETURNONLYFSDIRS;
	m_bi.lpszTitle = pszTitle;
	m_bi.hwndOwner = hWnd;

	LPITEMIDLIST pItem = ::SHBrowseForFolder(&m_bi);
	if(pItem != 0)
	{
		if(::SHGetPathFromIDList(pItem, m_szPath))
		{
			szPath.Format(_T("%s"), m_szPath);
		}
	}
	return szPath;
}

inline Image* GetZoomImage(Image *srcImage, int nNewWidth, int nNewHeight)
{
	if(!srcImage)
	{
		return NULL;
	}

	Bitmap bmp(nNewWidth, nNewHeight);
	Graphics * buffergraphics = Graphics::FromImage(&bmp);	//创建一个内存图像
	if(buffergraphics)
	{
		buffergraphics->DrawImage(srcImage, RectF(0, 0, nNewWidth, nNewHeight), 0, 0, srcImage->GetWidth(), srcImage->GetHeight(), UnitPixel);
		delete buffergraphics;
		buffergraphics = NULL;
	}
	return bmp.Clone(Rect(0, 0, nNewWidth, nNewHeight), PixelFormatDontCare);
}

inline void GdiRectToRECT(Rect &srcRC, RECT &rc)
{
	rc.left = srcRC.GetLeft();
	rc.right = srcRC.GetRight();
	rc.top = srcRC.GetTop();
	rc.bottom = srcRC.GetBottom();
}

inline void GetDeskPPI (double& xRate, double& yRate)
{
	xRate = yRate = 1.0;

	HWND hwndDesktop = GetDesktopWindow ();
	HDC  hdc = ::GetWindowDC (hwndDesktop);
	if (hdc != NULL)
	{
		xRate = ::GetDeviceCaps (hdc, LOGPIXELSX) * 1.0;
		yRate = ::GetDeviceCaps (hdc, LOGPIXELSY) * 1.0;
		::ReleaseDC (hwndDesktop, hdc);	
	}
}

inline int GetFontHeight(int nFontSize)
{
	double xRate, yRate = 0;
	::GetDeskPPI (xRate, yRate);

	int nOldPointSize = int((72.0 * nFontSize - 36) / 96.0);
	int nNewPointSize = nOldPointSize * (96.0 / yRate);
	int lfHeight = -(int)((nNewPointSize*((double)yRate)/72.0) + 0.5);
	return lfHeight;
}

inline BOOL IsFolderValid(CString strPath)
{
	CString sz = strPath;
	sz.Trim();
	if( (sz.IsEmpty()) || (_T("\\") == sz.Left(1)) || (_T(".") == sz.Left(1)) )
	{
		return FALSE;
	}
	if(::PathIsRelative(sz))
	{
		return FALSE;
	}
	return TRUE;	
}

inline BOOL IsFolderExist(CString strPath)
{
	if (strPath.Right(1) != _T("\\"))
	{
		strPath += _T("\\");
	}
	return ::PathFileExists(strPath);
}

inline BOOL CreateFolder(CString szFolder)
{
	BOOL bRet = TRUE;
	if(szFolder.Trim().IsEmpty())
	{
		return FALSE;
	}

	// 加最后一个字符_T("\\")
	CString szDir = szFolder.Trim();
	if (szDir.Right(1) != _T("\\"))
	{
		szDir += _T("\\");
	}

	if(ERROR_SUCCESS != ::SHCreateDirectoryEx(NULL, szDir, NULL))
	{
		bRet = FALSE;
	}
	return bRet;
}


inline CString FormatSpeed(int nBytes)
{
	CString szSpeed;
	double speed = 0.0;
	if(nBytes < 0)
	{
		szSpeed = _T("0 Bytes/s");
	}
	else if(nBytes >= 0 && nBytes < 1024)
	{
		szSpeed.Format(_T("%d Bytes/s"), nBytes);
	}
	else if(nBytes >= 1024 && nBytes < 1024*1024)
	{
		speed = nBytes / 1024.0;
		szSpeed.Format(_T("%.2f KB/s"), speed);
	}
	else if(nBytes >= 1024*1024)
	{
		speed = nBytes / (1024.0*1024.0);
		szSpeed.Format(_T("%.2f M/s"), speed);
	}
	return szSpeed;
}

inline CString FormatLeftTime(int nSpeed, __int64 nLeftSize)
{
	CString szTime;
	if(nSpeed <= 0)
	{
		szTime = _T("> 1 days");
	}
	else
	{
		CTime t1 = CTime::GetCurrentTime();
		CTime t2(t1.GetTime() + nLeftSize / nSpeed);
		CTimeSpan time = t2 - t1;


		CString sz;
		if(time.GetDays() > 0)
		{
			sz.Format(_T("%d days "), time.GetDays());
			szTime += sz;
		}
		if(time.GetHours() > 0)
		{
			sz.Format(_T("%d hours "), time.GetHours());
			szTime += sz;
		}
		if(time.GetMinutes() > 0)
		{
			sz.Format(_T("%d min "), time.GetMinutes());
			szTime += sz;
		}
		if(time.GetSeconds() >= 0)
		{
			sz.Format(_T("%d s"), time.GetSeconds());
			szTime += sz;
		}
	}
	return szTime;
}


inline CString FormatSize(__int64 nSize)
{
	CString sz;
	double f_size = 0.0;
	if(nSize < 0)
	{
		sz = _T("0 Bytes");
	}
	else if(nSize >= 0 && nSize < 1024)
	{
		sz.Format(_T("%I64d Bytes"), nSize);
	}
	else if(nSize >= 1024 && nSize < 1024*1024)
	{
		f_size = nSize / 1024.0;
		sz.Format(_T("%.2f KB"), f_size);
	}
	else if(nSize >= 1024*1024 && nSize < 1024*1024*1024)
	{
		f_size = nSize / (1024.0*1024.0);
		sz.Format(_T("%.2f M"), f_size);
	}
	else if(nSize >= 1024*1024*1024)
	{
		f_size = nSize / (1024.0*1024.0*1024.0);
		sz.Format(_T("%.2f G"), f_size);
	}
	return sz;
}

inline CString GetExeRunDirectory(HMODULE hModule)
{
	CString m_szFilePath = _T("");
	TCHAR lpFilePath[MAX_PATH] = {0};
	::GetModuleFileName(hModule, lpFilePath, MAX_PATH );
	m_szFilePath = lpFilePath;	
	m_szFilePath = m_szFilePath.Left(m_szFilePath.ReverseFind(_T('\\'))+1);
	return m_szFilePath;
}

inline CString StringFromIDResource(HINSTANCE hInst, UINT nID)
{
	//根据nID判断是否是ID_TEXT_lang_Window_title，来决定是否返回服务器的产品名称
	if(nID == ID_TEXT_lang_Window_title && g_pid_info.product_name_ != _T(""))
	{
		return g_pid_info.product_name_.c_str();
	}

	CString sz;
	TCHAR buffer[512] = {0};
	if(::LoadString(hInst, nID, buffer, 512))
	{
		sz.Format(_T("%s"), buffer);
	}
	return sz;
}

//inline BOOL IsWindowsVistaorLater()
//{
//	BOOL bIsWindowsVistaorLater = FALSE;
//	OSVERSIONINFO osvi;
//	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
//	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
//
//	if( ::GetVersionEx(&osvi) )
//	{
//		if ( osvi.dwMajorVersion >= 6 && osvi.dwMinorVersion >= 0 )
//		{
//			bIsWindowsVistaorLater = TRUE;
//		}
//	}
//	return bIsWindowsVistaorLater;
//}

inline DWORD RunExe(LPCTSTR lpFile, LPCTSTR lpParameters, bool isHide=false, bool isBlock=false)
{
	HANDLE hFile = ::CreateFile(lpFile,	// file to open
		GENERIC_READ,			// open for read
		FILE_SHARE_READ,		// share for reading
		NULL,					// default security
		OPEN_EXISTING,			// existing file only
		FILE_ATTRIBUTE_NORMAL,	// normal file
		NULL);					// no attr. template
	if (hFile != INVALID_HANDLE_VALUE) 
	{
		HANDLE  handle;

		if(IsWindowsVistaorLater())
		{
			SHELLEXECUTEINFO shExecInfo = {0};
			shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			shExecInfo.hwnd = NULL;
			shExecInfo.lpVerb = _T("runas");
			//shExecInfo.lpVerb = _T("open");
			shExecInfo.lpFile = lpFile;
			shExecInfo.lpParameters = lpParameters;
			shExecInfo.lpDirectory = NULL;
			if(isHide){
				shExecInfo.nShow = SW_HIDE;
				shExecInfo.lpParameters = _T("/verysilent /norestart");
			}else{
				shExecInfo.nShow = SW_SHOWNORMAL;
			}
			shExecInfo.hInstApp = NULL;
			 ::ShellExecuteEx(&shExecInfo);
			 handle = shExecInfo.hProcess;
		}
		else
		{			
			SHELLEXECUTEINFO shExecInfo = {0};
			shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			shExecInfo.hwnd = NULL;
			shExecInfo.lpVerb = _T("open");
			shExecInfo.lpFile = lpFile;
			shExecInfo.lpParameters = lpParameters;
			shExecInfo.lpDirectory = NULL;
			if(isHide){
				shExecInfo.nShow = SW_HIDE;
				shExecInfo.lpParameters = _T("/verysilent /norestart");
			}else{
				shExecInfo.nShow = SW_SHOWNORMAL;
			}
			shExecInfo.hInstApp = NULL;
			::ShellExecuteEx(&shExecInfo);
			handle = shExecInfo.hProcess;
		}
		//else
		//{
		//	if(isHide)
		//	{
		//		handle = ::ShellExecute(NULL, _T("open"), lpFile, _T("/verysilent"), NULL, SW_HIDE);
		//	}
		//	else
		//	{
		//		handle = ::ShellExecute(NULL, _T("open"), lpFile, lpParameters, NULL, SW_SHOWNORMAL);
		//	}
		//
		//}
		if(isBlock){
			WaitForSingleObject(handle, INFINITE);
		}
		DWORD rtnCode;
		GetExitCodeProcess(handle,&rtnCode);
		return rtnCode;
	}
	return -1;
}

inline DWORD RunCmd(LPCTSTR lpFile, LPCTSTR lpParameters, bool isHide=false, bool isBlock=false)
{
	HANDLE  handle;

	if(IsWindowsVistaorLater())
	{
		SHELLEXECUTEINFO shExecInfo = {0};
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd = NULL;
		shExecInfo.lpVerb = _T("runas");
		shExecInfo.lpFile = lpFile;
		shExecInfo.lpParameters = lpParameters;
		shExecInfo.lpDirectory = NULL;
		if(isHide){
			shExecInfo.nShow = SW_HIDE;
			//shExecInfo.lpParameters = _T("/verysilent /norestart");
		}else{
			shExecInfo.nShow = SW_SHOWNORMAL;
		}
		shExecInfo.hInstApp = NULL;
		::ShellExecuteEx(&shExecInfo);
		handle = shExecInfo.hProcess;
	}
	else
	{			
		SHELLEXECUTEINFO shExecInfo = {0};
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd = NULL;
		shExecInfo.lpVerb = _T("open");
		shExecInfo.lpFile = lpFile;
		shExecInfo.lpParameters = lpParameters;
		shExecInfo.lpDirectory = NULL;
		if(isHide){
			shExecInfo.nShow = SW_HIDE;
			//shExecInfo.lpParameters = _T("/verysilent /norestart");
		}else{
			shExecInfo.nShow = SW_SHOWNORMAL;
		}
		shExecInfo.hInstApp = NULL;
		::ShellExecuteEx(&shExecInfo);
		handle = shExecInfo.hProcess;
	}
	//else
	//{
	//	if(isHide)
	//	{
	//		handle = ::ShellExecute(NULL, _T("open"), lpFile, _T("/verysilent"), NULL, SW_HIDE);
	//	}
	//	else
	//	{
	//		handle = ::ShellExecute(NULL, _T("open"), lpFile, lpParameters, NULL, SW_SHOWNORMAL);
	//	}
	//
	//}
	if(isBlock){
		WaitForSingleObject(handle, INFINITE);
	}
	DWORD rtnCode;
	GetExitCodeProcess(handle,&rtnCode);
	return rtnCode;
}

inline BOOL CenterTopLevelWindow(HWND hwnd)
{
	BOOL bRet = FALSE;
	if(hwnd)
	{
		RECT rc;
		if(::GetWindowRect(hwnd, &rc))
		{
			int nWidth  = rc.right - rc.left;
			int nHeight = rc.bottom - rc.top;
			int nPos_x = -1;
			int nPos_y = -1;
			int cx = ::GetSystemMetrics(SM_CXSCREEN);
			int cy = ::GetSystemMetrics(SM_CYSCREEN);
			if((cx != 0) && (cy != 0))
			{
				nPos_x = (cx - nWidth) / 2;
				nPos_y = (cy - nHeight) / 2;
			}
			if(nPos_x >= 0 && nPos_y >= 0)
			{
				::SetWindowPos(hwnd, NULL, nPos_x, nPos_y, -1, -1,SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
				bRet = TRUE;
			}
		}
	}
	return bRet;
}

// 获取某个文件的MD5值
inline CString GetFileMD5(wchar_t *file)
{
	CString szMD5;
	if(file)
	{
		// 测试文件是否存在
		HANDLE hFile = ::CreateFile(file,			// file to open
			GENERIC_READ,							// open for read
			FILE_SHARE_READ,						// share for reading
			NULL,									// default security
			OPEN_EXISTING,							// existing file only
			FILE_ATTRIBUTE_NORMAL,					// normal file
			NULL);									// no attr. template
		if (hFile != INVALID_HANDLE_VALUE) 
		{ 
			LARGE_INTEGER dwFileSize;
			DWORD dwHigh = 0;
			dwFileSize.LowPart = ::GetFileSize(hFile, &dwHigh);
			if(INVALID_FILE_SIZE != dwFileSize.LowPart)
			{
				dwFileSize.HighPart = dwHigh;

				// 循环读文件
				int nKit = 64*1024*1024; // 64M ***必须为64的整数倍
				char *buffer = new char[nKit];
				if(buffer)
				{
					memset(buffer, 0, nKit);

					// 初始化MD5类对象
					MD5 ob;
					unsigned char digest[16] = {0};
					MD5::MD5_CTX context;
					ob.MD5Init (&context);

					LARGE_INTEGER nSize;
					nSize.QuadPart = dwFileSize.QuadPart;
					LARGE_INTEGER nOffset;
					nOffset.QuadPart = 0;
					while(nSize.QuadPart > 0)
					{
						nOffset.LowPart = ::SetFilePointer (hFile, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);
						if(INVALID_SET_FILE_POINTER == nOffset.LowPart)
						{
							break;
						}
						else
						{
							DWORD dwNumberOfBytesRead = 0;
							if(::ReadFile(hFile, buffer, nKit, &dwNumberOfBytesRead, NULL))
							{
								// read success
								if(0 == dwNumberOfBytesRead)
								{
									// end of file
									break;
								}
								else
								{
									ob.MD5Update (&context, (unsigned char*)buffer, dwNumberOfBytesRead);
									nOffset.QuadPart += dwNumberOfBytesRead;
									nSize.QuadPart -= dwNumberOfBytesRead;
								}
							}
							else
							{
								break;
							}
						}
					}

					if(nOffset.QuadPart == dwFileSize.QuadPart)
					{
						ob.MD5Final (digest, &context);
						string sz = ob.ResultToHex(digest, true);
						wchar_t buf[33];
						a2w(sz.c_str(), buf, _countof(buf));
						szMD5 = buf;
						//szMD5.Format(_T("%s"), sz.c_str());
					}

					delete buffer;
					buffer = NULL;
				}
			}
			CloseHandle(hFile);			
		}
	}
	szMD5.MakeUpper();
	return szMD5;
}


// 
inline CString GetMainExePath()
{
	
	CString path;
	wchar_t szPath[MAX_PATH] = {0};
//#ifdef _DEBUG
//	int len = MAX_PATH;
//	GetCurrentDirectory(len,szPath);
//	//wsprintf(szPath,_T("%s\\%s"),szPath,_T("WondershareApplicationCenter.exe"));
//	wcscat(szPath, _T("\\WondershareApplicationCenter.exe"));
//	path = szPath;
//#else
	if(S_OK == ::SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, szPath))
	{
		path = szPath;
		//TODO
		path += _T("\\Wondershare\\Wondershare Application Center\\WondershareApplicationCenter.exe");
	}
//#endif

	return path;

}

