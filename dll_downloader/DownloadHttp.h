/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:24
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\DownloadHttp.h
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	DownloadHttp
	file ext:	h
	author:		tanyc
	
	purpose:	
*********************************************************************/

#pragma once
#include "DownloadBase.h"

class CDownloadHttp : public CDownloadBase
{
public:
	CDownloadHttp();
	virtual ~CDownloadHttp();
	BOOL ParseResponseString ( CString csResponseString, OUT BOOL &bRetryRequest , OUT BOOL &bContError);
private:
	BOOL RequestHttpData ( BOOL bGet, char *szTailData=NULL, int *pnTailSize=NULL );
	CTime ConvertHttpTimeString ( CString csTimeGMT );
	CString FindAfterFlagString(LPCTSTR lpszFoundStr, CString csOrg);
	DWORD GetResponseCode ( CString csLineText );
	BOOL SendRequest ( LPCTSTR lpszReq, OUT CString &csResponse, char *szTailData=NULL, int *pnTailSize=NULL );
	CString GetRequestStr ( BOOL bGet);
	BOOL CheckIfSupportResume();
	CString GetRequestStrForResume ( BOOL bGet);
	BOOL ParseResponseStringForResume ( CString csResponseString, OUT BOOL &bRetryRequest , OUT BOOL &bContError);

protected:
	virtual BOOL DownloadOnce();
	virtual BOOL GetRemoteSiteInfo_Pro ();
};
