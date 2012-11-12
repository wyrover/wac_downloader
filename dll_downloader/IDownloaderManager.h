#pragma once

#include <Windows.h>
#include "wac_downloader_i.h"
namespace ns_WAC
{
	//////////////////////////////////////////////////////////////////////////
	/************************************************************************/
	// 功能描述：
	//	1.多任务，多线程，支持断点续传
	//	2.支持协议类型 : FTP , HTTP
	/************************************************************************/
	//////////////////////////////////////////////////////////////////////////
	// 任务属性
	const int MAX_URL_LENGTH = 512;
	struct DownloadTask
	{
		int			nID;				//任务id

		__int64     cbTotle,			//下载文件的总大小
			cbCurPos;           //已经下载文件的大小
		int			cbSpeedPerSecond;	//下载的速度
		wchar_t		szUrl[MAX_URL_LENGTH];         //下载任务的url
		wchar_t		szSavePath[MAX_URL_LENGTH];    //下载任务的保存路径
		wchar_t		szSaveFileName[MAX_URL_LENGTH];//下载任务保存的文件名
	};

	typedef enum NsResultEnum
	{
		RS_SUCCESS = 0,						// Operation success
		RS_NO_NEED_DOWNLOAD,                // 无需下载

		RS_ERR_CONNECT_FAILED,				// 连接失败
		RS_ERR_RCV_FAILED,					// 接收失败
		RS_ERR_SEND_FAILED,					// 发送失败
		RS_ERR_GET_WEBINFO_FAILED,          // 获取站点信息失败

		RS_ERR_TASK_ALLOC_FAILED,           // 任务分配失败
		RS_ERR_BUFSIZE,						// Buffer size out of band
		RS_INVALID_PARAMETER,				// 参数非法
		RS_ERR_FILE_OPRATION_FAILD,			// 文件操作失败
		RS_ERR_DISK_SPACE_NOT_ENOUGH,		// 磁盘空间不够
		RS_ERR_MD5_CHECK_FAILED,			// MD5校验失败

		RS_ERR_FILE_SIZE_INCORRECT,			// 文件大小不正确（所有下载线程退出后，仍不能下载完成）
		RS_ERR_USER_CANCEL,					// 用户已取消
		RS_ERR_DOWNLOAD_FAILED,             // 下载失败
		RS_ERR_UNKOWN,
	};

	struct IDownloaderManagerCallBack;
	class IDownloaderManager
	{
	public:
		IDownloaderManager(void);
		virtual ~IDownloaderManager(void) = 0;

		//=============================================================================
		//
		// 函数功能：	暂停下载任务
		// 参数：		nTaskID：需要暂停的任务id
		// 返回值：		BOOL
		// 备注：		2012-9-6
		//
		//=============================================================================
		virtual BOOL		Pause(int nTaskID) = 0;

		//=============================================================================
		//
		// 函数功能：	继续下载任务
		// 参数：		nTaskID：需要继续下载的任务id
		// 返回值：		BOOL
		// 备注：		2012-9-6
		//
		//=============================================================================
		virtual BOOL		Continue(int nTaskID) = 0;

		//=============================================================================
		//
		// 函数功能：	添加下载任务
		// 参数：		pszUrl：         下载任务的url
		//      		pszSavePath：    下载任务的保存路径
		//      		pszSaveFileName：下载完成后的文件名
		//      		iPID：     下载任务PID
		// 返回值：		int:             下载任务的id，可以暂停和继续， 小于0任务添加失败
		// 备注：		2012-9-6
		//
		//=============================================================================
		virtual int			AddTask(const wchar_t *pszUrl,
									const wchar_t *pszSavePath,
									const wchar_t *pszSaveFileName,
									int	  iPID) = 0;

		//=============================================================================
		//
		// 函数功能：	取消一个下载任务，并删除相应文件
		// 参数：		nTaskID：需要删除的下载任务id
		// 返回值：		BOOL
		// 备注：		2012-9-6
		//
		//=============================================================================
		virtual BOOL		RemoveTask(int nTaskID) = 0;

		//=============================================================================
		//
		// 函数功能：	查询下载任务的相关属性
		// 参数：		nTaskID：		下载任务id
		//				DownloadTask*： 下载任务的属性结构体 ，须事先new好
		// 返回值：		BOOL
		// 备注：		2012-9-6
		//
		//=============================================================================
		virtual BOOL		QueryTask(int nTaskID, DownloadTask* sDownloadTask) = 0; 

		//=============================================================================
		//
		// 函数功能：	取消下载任务
		// 参数：		nTaskID：		下载任务id
		// 返回值：		BOOL
		// 备注：		2012-9-6
		//
		//=============================================================================
		virtual BOOL		Cancel(int nTaskID) = 0; 

		//=============================================================================
		//
		// 函数功能：	设置回调函数
		// 参数：		IDownloaderManagerCallBack类型指针
		// 返回值：		void
		// 备注：		2012-9-6
		//
		//=============================================================================
		virtual void		SetCallBack(IDownloaderManagerCallBack* pcb) = 0; 

		virtual void		SetCallBack(HWND hWnd, UINT nMsgID) = 0;
		virtual void		SetCallBack(ICallBack* call_back) = 0;
		virtual void		ReleaseCallBack() = 0;
	};

		//////////////////////////////////////////////////////////////////////////
	// 回调
	struct IDownloaderManagerCallBack
	{
		/*
		// 用户调用接口后， 程序的执行结果
		virtual BOOL OnDelTask(int nTaskID) {};								// 删除任务后，执行结果
		virtual BOOL OnPause(int nTaskID) {};								// 暂停任务后，执行结果
		virtual BOOL OnCotinue(int nTaskID) {};								// 继续任务后，执行结果
		*/


		virtual void OnGetFileName(int nTaskID, WCHAR *szFileName) {};		// 获取到文件名
		virtual void OnGetFileSize(int nTaskID, __int64 nTotalSize) {};		// 获取到文件大小

		// 任务执行状况（用户需要得到通知）
		virtual void OnUpdate(int nTaskID,
							  __int64 nCurPos,
							  __int64 nTotalSize,
							  int nSpeed) {};								// 任务下载进度变化

		virtual void OnComplete(int nTaskID) {};							// 任务下载完成
		virtual void OnFail(int nTaskID, NsResultEnum nsError) {};			// 任务下载失败

		virtual void OnTest(int nTaskID, int nThreadID, int nType, __int64 nData) {};
		virtual void onStart(int nTaskID) {};
	};
}

#ifdef DOWNLOADERMANAGER_EXPORTS
#define DOWNLOADERMANAGER_API extern "C" __declspec(dllexport)
#else
#define DOWNLOADERMANAGER_API extern "C" __declspec(dllimport)
#endif

DOWNLOADERMANAGER_API ns_WAC::IDownloaderManager* CreateDownloadMGR(void);
