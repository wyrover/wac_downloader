#pragma once
#include "idownloadermanager.h"
#include "PublicFunction.h"

namespace ns_WAC
{

	typedef struct _DownloadNotifyPara
	{
		int nIndex;
		int nSerialNum;
		UINT nNotityType;
		LPVOID lpNotifyData;
	} t_DownloadNotifyPara;

	struct NotifyData
	{
		int     nID;				//任务id
		__int64 cbTotle;			//下载文件的总大小
		__int64 cbCurPos;           //已经下载文件的大小
		int     cbSpeedPerSecond;	//下载的速度

		NotifyData()
		{
			nID = cbSpeedPerSecond = 0;
			cbTotle = cbCurPos = 0;
		}
	};

	class CTaskManager;

	enum
	{
		NOTIFY_TYPE_HAS_GOT_REMOTE_FILENAME,				// 已经取得远程站点文件名, 当被下载的文件被重定向时才发送该通知，lpNotifyData 为 LPCTSTR 类型的文件名字符串指针
		NOTIFY_TYPE_HAS_GOT_REMOTE_FILESIZE,				// 已经取得远程站点文件大小，lpNotifyData 为 int 类型的文件大小
		NOTIFY_TYPE_HAS_END_DOWNLOAD,						// 已经结束下载，lpNotifyData 为 ENUM_DOWNLOAD_RESULT 类型的下载结果
		NOTIFY_TYPE_HAS_GOT_NEW_DOWNLOADED_DATA,            // 已经下载到

		NOTIFY_TYPE_HAS_GOT_THREAD_WILL_DOWNLOAD_SIZE,		// 已经获取到某线程本次需要下载的大小，lpNotifyData 为 int 类型的需要下载的大小
		NOTIFY_TYPE_HAS_GOT_THREAD_DOWNLOADED_SIZE,			// 已经获取到某线程已下载的数据大小，lpNotifyData 为 int 类型的已下载的大小
		NOTIFY_TYPE_HAS_GOT_THREAD_CACHE_SIZE,			    // 已经获取到某线程已下载的缓存数据大小，lpNotifyData 为 int 类型的已下载的大小
		NOTIFY_TYPE_HAS_START,	//开始下载
	};

	typedef enum _DownloadResult
	{
		ENUM_DOWNLOAD_RESULT_SUCCESS,
		ENUM_DOWNLOAD_RESULT_FAILED,
	} ENUM_DOWNLOAD_RESULT;

	class IObserver
	{
	public:
		virtual void OnNotify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam) = 0;
	};

	class DownloaderManager : public IDownloaderManager, public IObserver
	{
	public:
		DownloaderManager(void);
		~DownloaderManager(void);

		//static IDownloaderManager* get_instance();

		BOOL Pause(int nTaskID);
		BOOL Continue(int nTaskID);
		int	AddTask(const wchar_t *pszUrl,
					const wchar_t *pszSavePath,
					const wchar_t *pszSaveFileName,
					int	  PID);
		BOOL RemoveTask(int nTaskID);
		BOOL QueryTask(int nTaskID, DownloadTask* sDownloadTask);
		BOOL Cancel(int nTaskID);
		virtual void		SetCallBack(IDownloaderManagerCallBack*);
		virtual void		SetCallBack(HWND hWnd, UINT nMsgID);
		virtual void		SetCallBack(ICallBack* call_back);
		virtual void		ReleaseCallBack();

		virtual unsigned	GetTaskCount();
		//virtual BOOL		GetAllTask(__out DownloadTask *pDownload, int nBufferSize);
		virtual BOOL		Clear();	

	private:
		//static IDownloaderManager* s_sinalton;

	private:
		void OnNotify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam);

		CTaskManager *m_pDownloadCntl;								// 下载管理类指针
		IDownloaderManagerCallBack *m_pcb;
		ICallBack* m_icb;
		HWND m_hCallbackWnd;										// 接收回调消息的窗口句柄
		UINT m_nCallBackMsgID;										// 窗口用于接收回调消息的消息ID
	};
}
