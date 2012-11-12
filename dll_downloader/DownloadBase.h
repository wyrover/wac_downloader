/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:23
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\DownloadBase.h
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	DownloadBase
	file ext:	h
	author:		tanyc
	
	purpose:	
*********************************************************************/
#pragma once
#include "CCustomizeSocket.h"
#include "DownLoaderManager.h"

#define SLEEP_RETURN_Down(x)\
{\
	if ( ::WaitForSingleObject ( m_hEvtEndModule, x ) == WAIT_OBJECT_0 )\
	return DownloadEnd(FALSE);\
}

typedef void (*FUNC_SaveDownloadInfo) ( int nIndex, __int64 nDownloadedSize, __int64 nSimpleSaveSize, WPARAM wParam );

// 缺省端口号
#define DEFAULT_HTTP_PORT			80
#define DEFAULT_HTTPS_PORT			443
#define DEFAULT_FTP_PORT			21
#define DEFAULT_SOCKS_PORT			6815

using namespace ns_WAC;

class CDownloadBase  
{
public:
	CDownloadBase();
	virtual ~CDownloadBase();

	void Notify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam);
	void Notify(int nIndex, int nSerialNum, UINT nNotityType, LPVOID lpNotifyData);
	bool SetObserver( IObserver *pObserver );
	

	BOOL ThreadIsRunning ();
	CString GetDownloadObjectFileName ( CString *pcsExtensionName=NULL );
	void Clear_Thread_Handle();
	void ResetVar();
	__int64 GetUndownloadBytes ();
	BOOL ThreadProc_Download();
	BOOL SetSaveFileName ( LPCTSTR lpszSaveFileName );
	__int64 Get_WillDownloadSize();
	void Set_WillDownloadSize ( __int64 nWillDownloadSize );
	__int64 Get_DownloadedSize();
	void Set_DownloadedSize ( __int64 nDownloadedSize );
	int Get_TempSaveBytes();
	void Set_TempSaveBytes ( int nTempSaveBytes );
	CString GetRemoteFileName ();
	BOOL SetDownloadUrl ( LPCTSTR lpszDownloadUrl );
	virtual BOOL Connect ();
	BOOL GetRemoteSiteInfo ();
	void SetAuthorization ( LPCTSTR lpszUsername, LPCTSTR lpszPassword );
	void SetReferer(LPCTSTR lpszReferer);
	void SetUserAgent(LPCTSTR lpszUserAgent);
	void Set_SaveDownloadInfo_Callback ( FUNC_SaveDownloadInfo Proc_SaveDownloadInfo, WPARAM wParam );
	virtual BOOL Download (
		__int64 nWillDownloadStartPos,
		__int64 nWillDownloadSize,
		__int64 nDownloadedSize
		);
	BOOL Is_SupportResume () { return m_bSupportResume; }
	CString Get_ProtocolType () { return m_csProtocolType; }
	time_t Get_TimeLastModified() { return m_TimeLastModified.GetTime(); }
	__int64 Get_FileTotalSize() { return m_nFileTotalSize; }
	CString Get_FileMD5_Web() { return m_csMD5_Web; }
	CString Get_UserName () { return m_csUsername; }
	CString Get_GetPassword () { return m_csPassword; }
	CString Get_DownloadUrl () { return m_csDownloadUrl; }
	BOOL Is_DownloadSuccess() { return m_bDownloadSuccess; }
	void Set_DownloadSuccess(BOOL bSuccess) { m_bDownloadSuccess = bSuccess; }
	HANDLE Get_Thread_Handle() { return m_hThread; }
	__int64 Get_WillDownloadStartPos() { return m_nWillDownloadStartPos; }
	CString Get_ServerName() { return m_csServer; }
	void StopDownload (DWORD dwTimeout = 1000);
	int m_nIndex;
	LPVOID m_pDownloadMTR;
	int m_nSerialNum;										//下载序列号的存储,初始化为-1
	int SetTaskID(int nPos);								//设置序列号
	int GetSerialNum();										//获取序列号
	BOOL IsCallBackEnable();

protected:
	virtual BOOL GetRemoteSiteInfo_Pro();
	virtual BOOL DownloadOnce();
	CString GetRefererFromURL();
	__int64 SaveDataToFile ( char *data, int size );
	virtual BOOL RecvDataAndSaveToFile(CCustomizeSocket &SocketClient,char *szTailData=NULL, int nTailSize=0);
	BOOL DownloadEnd ( BOOL bRes );

	HANDLE m_hfile;
	HANDLE m_hEvtEndModule;									// 模块结束事件
	CCustomizeSocket m_SocketClient;						// 连接服务器的 Socket
	CString		m_csDownloadUrl;							// 待下载URL
	CString		m_csSaveFileName;							// 保存的文件名
	FUNC_SaveDownloadInfo m_Proc_SaveDownloadInfo;			// 保存下载信息的回调函数
	WPARAM m_wSaveDownloadInfo_Param;
	BOOL		m_bSupportResume;							// 是否支持断点续传
	HANDLE		m_hThread;									// 下载线程句柄
	CTime		m_TimeLastModified;							// 文件日期(远程文件的信息)

	// 下载字节数
	CString                 m_csMD5_Web;                    // 站点提供的MD5
	__int64					m_nFileTotalSize;				// 文件总的大小，-1表示未知文件大小
	__int64					m_nWillDownloadStartPos;		// 要下载文件的开始位置
	__int64					m_nWillDownloadSize;			// 本次需要下载的大小，-1表示不知道文件大小，所以下载到服务器关闭连接为止
	CThreadGuard			m_CSFor_WillDownloadSize;		// 访问 m_nWillDownloadSize 变量的互斥锁
	int						m_nTempSaveBytes;				// 存放在临时缓冲中的字节数
	CThreadGuard			m_CSFor_TempSaveBytes;			// 访问 m_nTempSaveBytes 变量的互斥锁
	__int64					m_nDownloadedSize;				// 已下载的字节数，指完全写到文件中的字节数，不包含临时缓冲里的数据
	CThreadGuard			m_CSFor_DownloadedSize;			// 访问 m_nDownloadedSize 变量的互斥锁

	// URL header
	CString		m_csReferer;								// url Referer
	CString		m_csCookieFlag;								// url cookie
	CString		m_csUserAgent;								// url UserAgent
	CString		m_csUsername;								// 是否进行验证 : Request-Header: Authorization
	CString		m_csPassword;

	// 下载过程中所用的变量
	CString		m_csProtocolType;							// 所使用的传输协议：http、ftp等
	CString		m_csServer;
	CString		m_csObject;
	CString		m_csFileName;
	USHORT		m_nPort;

private:
	BOOL OpenFileForSave();
	BOOL m_bDownloadSuccess;
	IObserver *m_pObserver;
};

void url_Encode(std::wstring& str);
void url_Decode(CString& str);
int Base64Encode(LPCTSTR lpszEncoding, CString &strEncoded);
int Base64Decode(LPCTSTR lpszDecoding, CString &strDecoded);
BOOL ParseURLDIY(LPCTSTR lpszURL,CString& strServer,CString& strObject,USHORT& nPort, CString &csProtocolType);
