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
#define DEFAULT_THREAD_COUNT		 5								// Ĭ������һ���ļ����õ��߳��� 1
#define MAX_DOWNLOAD_THREAD_COUNT	(MAXIMUM_WAIT_OBJECTS - 1)		// ����һ���ļ����ʹ�õ��߳��� (64 - 1) (WaitForMultipleObjects)
#define RETRY_TIME 4
//
// ���������������Ϣ
//
typedef struct _downloadcellinfo
{
	__int64 nWillDownloadStartPos;			// Ҫ�����ļ��Ŀ�ʼλ��
	__int64 nWillDownloadSize;				// ������Ҫ���صĴ�С��-1��ʾһֱ���ص��ļ�β
	__int64 nDownloadedSize;				// ���߳������صĴ�С
} t_DownloadCellInfo;

struct ServerHost 
{
	std::wstring server_;
	int weight_;
};

//�ֶ�md5��Ϣ
struct PartInfo
{
	__int64 offset_;
	__int64 length_;
	std::wstring md5_;
};
//�ӷ�������ȡ��pid��Ϣ
struct CPIDInfo
{
	std::wstring downurl_;
	std::wstring md5_;
	std::vector<ServerHost> servers_;
	std::wstring md5_url_;
	std::vector<PartInfo> part_infos_;
};

//
// ������Ϣ
//
typedef struct _DownloadInfo
{
	DWORD dwThreadCount;					// ���ļ��ɶ��ٸ��߳�������
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
	int SetTaskID(int nSerialNum);			//�������к�
	int GetSerialNum();							//��ȡ���к�
	int CreatEndEvent();						//�����µĽ����¼�
	void EnableCallBack(BOOL bEnable = TRUE);
	BOOL IsCallBackEnable();
	void SetComplete(BOOL bComplete = TRUE);
	BOOL IsComplete();

	// MD5�ļ�У��
	void SetNeedCheckMD5(BOOL bCheck = FALSE);	// �����Ƿ���Ҫ��֤MD5
	BOOL IsNeedCheckMD5();						// �Ƿ���Ҫ��֤MD5
	void SetMD5_WebSite(CString szMD5);         // ����Ҫ�����ļ���MD5ֵ����Ϊ��׼ֵ
	CString GetMD5_WebSite();					// ��վ���ȡ����MD5ֵ
	CString GetFileMD5(wchar_t *file);             // ��ȡĳ���ļ���MD5ֵ

	HANDLE m_hEvtEndModule;						// ģ������¼�
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

	//�ӷ��������ص�url�����ѡ��һ��
	CString GetRandUrl();
	NsResultEnum RequestPIDInfo(LPCTSTR lpszDownloadURL);
	CString ReplaceHost(const CString& url, const CString& host);
	NsResultEnum getMd5Info(LPCTSTR downurl);
	BOOL CheckPartMD5();
	CString GetPartFileMD5(wchar_t *file, __int64 offset, __int64 length);
	std::wstring GetHost(const CString& url);
	//���md5��xml�ļ�ʱ���Ƿ��Ǵ���exe�ļ�ʱ�䣬�����ж�xml�Ƿ���Ч
	BOOL CheckLastModifyTime(LPCTSTR exe_url, LPCTSTR xml_url);

	__int64 m_nTotalDownloadedSize_ThisTimes;	// ��ʾ��������������������ܹ����ص��ֽ���
	CThreadGuard m_CSFor_DownloadedData;
	BOOL m_bForceDownload;
	HANDLE m_hThread; //�������̣߳�����ÿ�������������߳�
	DWORD m_dwDownloadStartTime;
	CString m_csSavePath, m_csSaveOnlyFileName, m_csSavePathFileName, m_csDownloadURL;
	CString m_csProtocolType;
	int m_nThreadCount;							// �߳���
	CDownloadBase *m_pDownloadPub_Info;			// ȡվ����Ϣ����
	CDownloadBase *m_pDownloadPub_MTR;			// ���߳����ض���
	t_BaseDownInfo m_BaseDownInfo;				// ���ػ�����Ϣ���߳�����
	t_DownloadCellInfo *m_pDownloadCellInfo;	// �������ض���Ĳ���
	BOOL m_EnableCallBack;						// �Ƿ�����ص�
	BOOL m_bComplete;							// �Ƿ��Ѿ��������
	BOOL m_bNeedCheckMD5;						// �Ƿ���֤MD5
	int m_nSerialNum;							//�������кŵĴ洢����ʼ��Ϊ-1����TASKID
	IObserver *m_pObserver;

	CString m_csMD5_Web; //���ȿ����Է�����httpͷ�����Ϊ��Content-MD5

	DWORD m_last_report_time;

	CPIDInfo m_pid_info;
	int m_PID; //down���������pid
};
