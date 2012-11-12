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

// ��������
typedef struct node
{
	void *info;	//headָ���info��Žڵ�����������Ĵ��CDownloadManagerָ��
	wchar_t szUrl[2048];			//���صĵ�ַ
	wchar_t szSavePath[2048];		//����·��
	wchar_t szSaveFileName[1024];	//�ļ���
	int iPID;//��ƷPID�����ڴӷ�����ȡ����
	struct node *link;
}NODE;

class CTaskManager
{
private:
	CDownloadManager *m_pCurDownloadMTR;
	int m_nCurDownloadMTRPos;		//��ɶ�ã�
	NODE *m_pHead;							 // ָ��ͷ����ָ��,Ϊ��infoָ�����һ���ռ�,����������¼����Ľ�����
	CThreadGuard m_csCntl;					 // ���ʹ�����
	BOOL m_bQuery;
	IObserver *m_pObserver;

	//int m_nSeq;//���кţ����������ڱ�ʶ����ID

public:
	CTaskManager();
	virtual ~CTaskManager();

	bool SetObserver( IObserver *pObserver );

	int GetTaskNum();
	int AddTask(const wchar_t *pszUrl, const wchar_t *pszSavePath, const wchar_t *pszSaveFileName, int PID);
	BOOL StopTask(int nPos);				//��ͣ
	BOOL MoveOnTask(int nPos);				//����
	BOOL RemoveAllTask();					//ɾ������
	BOOL RemoveTheTask(int nPos);			//ɾ��ָ������

	// ��ѯһ�������״̬
	BOOL QueryState( int nPos, 
		__int64 &nFileTotaleSize, 
		__int64 &nTotalDownloadedSize,
		__int64 &nTotalDownloadedSize_ThisTimes,
		int &nDownloadElapsedTime ); 
	int GetNodeInfo(int nPos, TCHAR *szUrl, TCHAR *szSavePath, TCHAR *szSaveFileName);	//��ѯ�ڵ���Ϣ
};
}
