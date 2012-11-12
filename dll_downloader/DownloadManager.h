/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:24
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\DownloadManager.h
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	DownloadManager
	file ext:	h
	author:		tanyc
	
	purpose:	
*********************************************************************/

#pragma once
#include <vector>
#include "DownloadFtp.h"
#include "DownloadHttp.h"
#define DEFAULT_THREAD_COUNT		 5								// 默认下载一个文件所用的线程数 1
#define MAX_DOWNLOAD_THREAD_COUNT	(MAXIMUM_WAIT_OBJECTS - 1)		// 下载一个文件最大使用的线程数 (64 - 1) (WaitForMultipleObjects)
#define RETRY_TIME 4
//
// 单个对象的下载信息
//
typedef struct _downloadcellinfo
{
	__int64 nWillDownloadStartPos;			// 要下载文件的开始位置
	__int64 nWillDownloadSize;				// 本次需要下载的大小，-1表示一直下载到文件尾
	__int64 nDownloadedSize;				// 该线程已下载的大小
} t_DownloadCellInfo;

struct ServerHost 
{
	std::wstring server_;
	int weight_;
};

//分段md5信息
struct PartInfo
{
	__int64 offset_;
	__int64 length_;
	std::wstring md5_;
};
//从服务器获取的pid信息
struct CPIDInfo
{
	std::wstring downurl_;
	std::wstring md5_;
	std::vector<ServerHost> servers_;
	std::wstring md5_url_;
	std::vector<PartInfo> part_infos_;
};

//
// 下载信息
//
typedef struct _DownloadInfo
{
	DWORD dwThreadCount;					// 该文件由多少个线程在下载
} t_BaseDownInfo;

class CDownloadManager
{
public:
	CDownloadManager();
	virtual ~CDownloadManager();
	void Notify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam);
	void Notify(int nIndex, int nSerialNum, UINT nNotityType, LPVOID lpNotifyData);
	bool SetObserver( IObserver *pObserver );

	__int64 Get_TotalDownloadedSize_ThisTimes ();
	__int64 Get_TotalDownloadedSize();
	__int64 Get_FileTotaleSize();
	static CString GetLocalFileNameByURL ( LPCTSTR lpszDownloadURL );
	void StopDownload(DWORD dwTimeout = 5000);
	DWORD GetDownloadElapsedTime ();
	BOOL Download (
		LPCTSTR lpszDownloadURL,
		LPCTSTR lpszSavePath,
		LPCTSTR lpszSaveOnlyFileName,
		int iPID,
		LPCTSTR lpszUsername=NULL,
		LPCTSTR lpszPassword=NULL,
		BOOL bForceDownload=FALSE
		);
	BOOL SetThreadCount ( int nThreadCount );
	void Callback_SaveDownloadInfo ( int nIndex, __int64 nDownloadedSize, __int64 nSimpleSaveSize );
	BOOL ThreadProc_DownloadMTR();
	int SetTaskID(int nSerialNum);			//设置序列号
	int GetSerialNum();							//获取序列号
	int CreatEndEvent();						//创建新的结束事件
	void EnableCallBack(BOOL bEnable = TRUE);
	BOOL IsCallBackEnable();
	void SetComplete(BOOL bComplete = TRUE);
	BOOL IsComplete();

	// MD5文件校验
	void SetNeedCheckMD5(BOOL bCheck = FALSE);	// 设置是否需要验证MD5
	BOOL IsNeedCheckMD5();						// 是否需要验证MD5
	void SetMD5_WebSite(CString szMD5);         // 设置要下载文件的MD5值，此为基准值
	CString GetMD5_WebSite();					// 从站点获取到的MD5值
	CString GetFileMD5(wchar_t *file);             // 获取某个文件的MD5值

	HANDLE m_hEvtEndModule;						// 模块结束事件
private:
	void StandardSaveFileName ();
	int GetDownloadInfoWholeSize();
	int FindIndexByThreadHandle ( HANDLE hThread );
	NsResultEnum WaitForDownloadFinished ();
	BOOL GetDownloadResult ();
	BOOL AttemperDownloadTask ( int nIndex );
	int GetUndownloadMaxBytes ( __int64 &nUndownloadBytes );
	BOOL HandleDownloadFinished ( NsResultEnum& eDownloadResult );
	BOOL SaveDownloadInfo ();
	BOOL AssignDownloadTask ();
	BOOL DownloadInfoIsValid ();
	BOOL ReadDownloadInfo ();
	CString GetTempFilePath ();
	NsResultEnum StartMTRDownload ();
	void DeleteDownloadObjectAndDataMTR();
	void DeleteDownloadObject_Info();
	BOOL CreateDownloadObjectAndDataMTR ();
	CDownloadBase* CreateDownloadObject ( int nCount = 1 );
	void DeleteDownloadObject ( CDownloadBase *pDownloadPub );

	//从服务器返回的url中随机选择一个
	CString GetRandUrl();
	NsResultEnum RequestPIDInfo(LPCTSTR lpszDownloadURL);
	CString ReplaceHost(const CString& url, const CString& host);
	NsResultEnum getMd5Info(LPCTSTR downurl);
	BOOL CheckPartMD5();
	CString GetPartFileMD5(wchar_t *file, __int64 offset, __int64 length);
	std::wstring GetHost(const CString& url);
	//检查md5的xml文件时间是否是大于exe文件时间，用于判断xml是否有效
	BOOL CheckLastModifyTime(LPCTSTR exe_url, LPCTSTR xml_url);

	__int64 m_nTotalDownloadedSize_ThisTimes;	// 表示这次启动下载任务以来总共下载的字节数
	CThreadGuard m_CSFor_DownloadedData;
	BOOL m_bForceDownload;
	HANDLE m_hThread; //下载主线程，管理每个真正的下载线程
	DWORD m_dwDownloadStartTime;
	CString m_csSavePath, m_csSaveOnlyFileName, m_csSavePathFileName, m_csDownloadURL;
	CString m_csProtocolType;
	int m_nThreadCount;							// 线程数
	CDownloadBase *m_pDownloadPub_Info;			// 取站点信息对象
	CDownloadBase *m_pDownloadPub_MTR;			// 多线程下载对象
	t_BaseDownInfo m_BaseDownInfo;				// 下载基本信息，线程数等
	t_DownloadCellInfo *m_pDownloadCellInfo;	// 各个下载对象的参数
	BOOL m_EnableCallBack;						// 是否允许回调
	BOOL m_bComplete;							// 是否已经下载完成
	BOOL m_bNeedCheckMD5;						// 是否验证MD5
	int m_nSerialNum;							//下载序列号的存储，初始化为-1，即TASKID
	IObserver *m_pObserver;

	CString m_csMD5_Web; //优先考虑以服务器http头里面的为主Content-MD5

	DWORD m_last_report_time;

	CPIDInfo m_pid_info;
	int m_PID; //down函数传入的pid
};
