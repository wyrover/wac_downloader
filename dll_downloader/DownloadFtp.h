// DownloadFtp.h: interface for the CDownloadFtp class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "DownloadBase.h"

class CDownloadFtp : public CDownloadBase
{
public:
	CDownloadFtp();
	virtual ~CDownloadFtp();
	BOOL Connect ();

private:
	void GetFileTimeInfo ( CString csFileTime1, CString csFileTime2, CString csFileTime3 );
	BOOL CreateFTPDataConnect ( CCustomizeSocket &SocketClient );
	void ParseFileInfoStr ( CString &csFileInfoStr );

protected:
	virtual BOOL GetRemoteSiteInfo_Pro ();
	virtual BOOL DownloadOnce();
};
