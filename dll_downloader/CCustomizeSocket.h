/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:22
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\CCustomizeSocket.h
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	CCustomizeSocket
	file ext:	h
	author:		tanyc
	
	purpose:	
*********************************************************************/
#pragma once
#include <vector>
#include <atlstr.h>
#include <atltime.h>
#include "PublicFunction.h"
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

#define NET_BUFFER_SIZE			4096		// Ä¬ÈÏ»º³å´óÐ¡

typedef std::map<std::wstring, std::vector<ULONG> > Host2MultiIpMap;
class CCustomizeSocket
{
public:
	CCustomizeSocket(void);
	virtual ~CCustomizeSocket();

	static ULONG GetHost(LPCTSTR lpszHost);
	static void ClearHost();

	BOOL Connect ( LPCTSTR lpszHost, USHORT nPort ,DWORD dwTimeout = INFINITE);
	BOOL ReConnect ();
	void Disconnect();
	BOOL Is_Connected() { return m_bConnected; }
	int Receive ( char *szBuf, int size, BOOL bBlock=TRUE );
	BOOL Send ( char *data, int size );
	BOOL SendString ( LPCTSTR lpszData, ... );
	BOOL SendString2 ( LPCTSTR lpszData );
	int GetResponse ( CString *pcsResponseStr = NULL, BOOL bBlock = TRUE );
	BOOL GetResponse ( int nVerifyCode, CString *pcsResponseStr = NULL );
	CString GetResponseHistoryString () { return m_csResponseHistoryString; }
	void ClearResponseHistoryString () { m_csResponseHistoryString.Empty(); }
	CString GetDigitStrAtHead ( LPCTSTR lpszStr );
	BOOL GetIPAndPortByPasvString ( LPCTSTR lpszPasvString, OUT CString &csIP, OUT USHORT &nPort );
	void SetEventOfEndModule ( HANDLE hEvtEndModule );

	SOCKET m_socket;
	int m_nIndex;
protected:
	BOOL InitSocket();

private:
	BOOL m_bConnected;
	CString m_csResponseHistoryString;
	HANDLE m_hEvtEndModule;
	CString m_szHost;
	USHORT  m_nPort;
	DWORD   m_dwTimeout;

	static Host2MultiIpMap m_addr_host;
};