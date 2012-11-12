/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:24
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\TaskManager.h
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	TaskManager
	file ext:	h
	author:		tanyc
	
	purpose:	
*********************************************************************/
#pragma once
#include "DownloadManager.h"
#define MULTASK_INVALID_ID -1

namespace ns_WAC
{

// 类链表结点
typedef struct node
{
	void *info;	//head指针的info存放节点个数，其他的存放CDownloadManager指针
	wchar_t szUrl[2048];			//下载的地址
	wchar_t szSavePath[2048];		//保存路径
	wchar_t szSaveFileName[1024];	//文件名
	int iPID;//产品PID，用于从服务器取数据
	struct node *link;
}NODE;

class CTaskManager
{
private:
	CDownloadManager *m_pCurDownloadMTR;
	int m_nCurDownloadMTRPos;		//有啥用？
	NODE *m_pHead;							 // 指向头结点的指针,为其info指针分配一个空间,可以用来记录链表的结点个数
	CThreadGuard m_csCntl;					 // 访问共享锁
	BOOL m_bQuery;
	IObserver *m_pObserver;

	//int m_nSeq;//序列号，递增，用于标识任务ID

public:
	CTaskManager();
	virtual ~CTaskManager();

	bool SetObserver( IObserver *pObserver );

	int GetTaskNum();
	int AddTask(const wchar_t *pszUrl, const wchar_t *pszSavePath, const wchar_t *pszSaveFileName, int PID);
	BOOL StopTask(int nPos);				//暂停
	BOOL MoveOnTask(int nPos);				//继续
	BOOL RemoveAllTask();					//删除所有
	BOOL RemoveTheTask(int nPos);			//删除指定任务

	// 查询一个任务的状态
	BOOL QueryState( int nPos, 
		__int64 &nFileTotaleSize, 
		__int64 &nTotalDownloadedSize,
		__int64 &nTotalDownloadedSize_ThisTimes,
		int &nDownloadElapsedTime ); 
	int GetNodeInfo(int nPos, TCHAR *szUrl, TCHAR *szSavePath, TCHAR *szSaveFileName);	//查询节点信息
};
}
