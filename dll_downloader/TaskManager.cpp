/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:26
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\TaskManager.cpp
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	TaskManager
	file ext:	cpp
	author:		tanyc
	
	purpose:	
*********************************************************************/

#include "stdafx.h"
#include "TaskManager.h"
using namespace ns_WAC;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTaskManager::CTaskManager()
{
	/* ����ͷ��� create head node */
	m_pHead = (NODE *)new NODE; 
	m_pHead->info = (void*)new int;
	*(int*)m_pHead->info = 0;
	m_pHead->link = NULL;

	m_pCurDownloadMTR = NULL;
	m_nCurDownloadMTRPos = 0;
	m_bQuery = TRUE;
	m_pObserver = NULL;
	//m_nSeq = 1;
}

CTaskManager::~CTaskManager()
{
	/* ɾ��ͷ��� delete head node */
	if( m_pHead )
	{
		delete m_pHead->info;
		m_pHead->info = NULL;

		delete m_pHead;
		m_pHead = NULL;
	}
}

int CTaskManager::GetTaskNum()
{
	return *(int*)m_pHead->info;
	//int seq = 0;
	//NODE *loop = m_pHead;
	//while(loop->link)
	//{
	//	loop = loop->link;
	//	seq++;
	//}
	//return seq;
}

int CTaskManager::AddTask(const wchar_t *pszUrl, const wchar_t *pszSavePath, const wchar_t *pszSaveFileName, int PID)
{
	//����ʹ��PID��������Ϊ�˷�ֹ�������һ�𣬼�ɾ��ԭ������
	RemoveTheTask(PID);

	m_csCntl.Lock();
	// �¼���������������еĳ�ʼλ��
	int nPos = 1;
	m_pCurDownloadMTR = new CDownloadManager;

	//���ݹ۲���ָ��
	m_pCurDownloadMTR->SetObserver(m_pObserver);

	m_pCurDownloadMTR->SetNeedCheckMD5(TRUE);
	m_pCurDownloadMTR->SetMD5_WebSite(_T(""));

	// ����һ���½��
	NODE *pNew = new NODE;
	memset(pNew, 0, sizeof(pNew));

	pNew->info = (void*)m_pCurDownloadMTR;
	pNew->link = NULL;

	// �ҵ�����λ��,������
	NODE *loop = m_pHead;
	while(loop->link)
	{
		loop = loop->link;
		nPos++;
	}
	loop->link = pNew;

	// ���������
	(*(int*)m_pHead->info)++;

	//������֮ǰҪ���������к�
	m_pCurDownloadMTR->SetTaskID(PID);

	//��ʼ����ǰ��¼������Ϣ��֧�ֶϵ�����
	pNew->iPID = PID;

	if(pszUrl != NULL)
	{
		wcscpy_s(pNew->szUrl, _countof(pNew->szUrl), pszUrl);
	}

	if(pszSavePath == NULL)
	{
		wchar_t lpPathBuffer[2 * MAX_PATH] = {0};
		DWORD dwRetVal = ::GetTempPath(sizeof(lpPathBuffer), lpPathBuffer);
		if ((dwRetVal > sizeof(lpPathBuffer)) || (dwRetVal == 0))
		{
			return -1;
		}
		wcscpy_s(pNew->szSavePath, _countof(pNew->szSavePath), lpPathBuffer);
	}
	else
	{
		wcscpy_s(pNew->szSavePath, _countof(pNew->szSavePath), pszSavePath);
	}
	
	if(pszSaveFileName != NULL)
	{
		wcscpy_s(pNew->szSaveFileName, _countof(pNew->szSaveFileName), pszSaveFileName);
	}

	// ��ʼ����
	BOOL ret = m_pCurDownloadMTR->Download( pszUrl, pszSavePath, pszSaveFileName, PID);

	//���浱ǰ�����������λ��
	m_nCurDownloadMTRPos = nPos;

	m_csCntl.Unlock();

	CString szLog;
	szLog.Format(_T("add task: %d %s"), PID, ret ? _T("succ") : _T("fail"));
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);	
	
	return ret ? PID : MULTASK_INVALID_ID;
}

//////////////////////////////////////////////////////////////////////////
// ɾ��ָ������������
//////////////////////////////////////////////////////////////////////////
BOOL CTaskManager::RemoveTheTask(int nPos)
{
	//if( nPos < 1 || nPos > GetTaskNum() )
	//{
	//	// ѡ������񳬳���Χ
	//	return FALSE;
	//}

	//��ͣquery�Ĳ��� 
	m_bQuery = FALSE;
	m_csCntl.Lock();
	m_nCurDownloadMTRPos = nPos;

	NODE * pre_loop = m_pHead;
	NODE *loop = m_pHead->link;
	while(loop != NULL)
	{
		CDownloadManager *tmp = (CDownloadManager*)loop->info;
		if(tmp->GetSerialNum() == nPos)
		{
			break;
		}
		pre_loop = pre_loop->link;
		loop = loop->link;
	}

	//NODE *loop = m_pHead;
	//for( int i = 1; i < nPos; i++ )
	//{
	//	loop = loop->link;
	//}

	if(loop)
	{

		// ָֹͣ������������
		m_pCurDownloadMTR = (CDownloadManager*)loop->info;
		if (m_pCurDownloadMTR)
		{
			m_pCurDownloadMTR->EnableCallBack(FALSE);
			m_pCurDownloadMTR->StopDownload();
			delete m_pCurDownloadMTR;
			m_pCurDownloadMTR = NULL;

			// ��������
			pre_loop->link = loop->link;
			delete loop;
			loop = NULL;

			// ������ǰָ�������,���ȼ���[��һ������,ǰһ������]
			if( 1 == *(int*)m_pHead->info )  // ���ɾ���������һ������
			{
				*(int*)m_pHead->info = 0;
				m_nCurDownloadMTRPos = 0;
				m_pCurDownloadMTR = NULL;
			}
			else
			{
				if( pre_loop->link )  // ���ں�һ������
				{
					m_pCurDownloadMTR = (CDownloadManager*)pre_loop->link->info;
				}
				//��Ȼ��ָ��ǰ����
				else
				{
					m_pCurDownloadMTR = (CDownloadManager*)pre_loop->info;
					m_nCurDownloadMTRPos--;
				}

				//����������һ��
				(*(int*)m_pHead->info)--;
			} // End of if( 1 == *(int*)m_pHead->info )
		}

	}
	m_csCntl.Unlock();
	m_bQuery = TRUE;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
// ɾ�����е����������ͷſռ�
//////////////////////////////////////////////////////////////////////////
BOOL CTaskManager::RemoveAllTask()
{
	if( 0 == m_nCurDownloadMTRPos )
	{
		return FALSE;
	}

	//��ͣquery�Ĳ��� 
	m_bQuery = FALSE;
	m_csCntl.Lock();
	NODE *loop = m_pHead->link;
	int nTaskNum = GetTaskNum();
	if (nTaskNum > 0)
	{
		while(loop)
		{
			NODE *pOldNode = loop;
			CDownloadManager *p = (CDownloadManager*)loop->info;
			if(p)
			{
				p->EnableCallBack(FALSE);
				p->StopDownload();
				delete p;
				p = NULL;
			}

			loop = loop->link;

			delete pOldNode;
			pOldNode = NULL;
			(*(int*)m_pHead->info)--;
		}
	}
	m_csCntl.Unlock();
	m_bQuery = TRUE;
	return TRUE;
}


BOOL CTaskManager::QueryState( int nPos, 
							  __int64 &nFileTotaleSize, 
							  __int64 &nTotalDownloadedSize,
							  __int64 &nTotalDownloadedSize_ThisTimes,
							  int &nDownloadElapsedTime )
{
	//if( nPos < 1 || nPos > GetTaskNum() )
	//{
	//	return FALSE;  // ѡ������񳬳���Χ
	//}
	if (!m_bQuery)
	{
		return FALSE;
	}

	BOOL nRet = FALSE;
	m_csCntl.Lock();

	NODE *loop = m_pHead->link;
	while(loop != NULL)
	{
		CDownloadManager *tmp = (CDownloadManager*)loop->info;
		if(tmp->GetSerialNum() == nPos)
		{
			break;
		}
		loop = loop->link;
	}

	//for( int i = 0; i < nPos; i++ )
	//{
	//	loop = loop->link;
	//}

	if(loop)
	{
		CDownloadManager *tmp = (CDownloadManager*)loop->info;
		if(tmp)
		{
			nDownloadElapsedTime = tmp->GetDownloadElapsedTime();
			nFileTotaleSize = tmp->Get_FileTotaleSize();
			if( nFileTotaleSize > 0 )
			{
				nTotalDownloadedSize = tmp->Get_TotalDownloadedSize();
				nTotalDownloadedSize_ThisTimes = tmp->Get_TotalDownloadedSize_ThisTimes();
			}
			nRet = TRUE;
		}
	}
	else
	{
		nRet = FALSE;
	}

	m_csCntl.Unlock();
	return nRet;
}

//��ͣ����
BOOL CTaskManager::StopTask(int nPos)
{
	//if( nPos < 1 || nPos > GetTaskNum() )
	//{
	//	// ѡ������񳬳���Χ
	//	return FALSE;
	//}

	//��ͣquery�Ĳ��� 
	m_bQuery = FALSE;
	m_csCntl.Lock();

	NODE *loop = m_pHead->link;
	while(loop != NULL)
	{
		CDownloadManager *tmp = (CDownloadManager*)loop->info;
		if(tmp->GetSerialNum() == nPos)
		{
			break;
		}
		loop = loop->link;
	}
	//for( int i = 0; i < nPos; i++ )
	//{
	//	loop = loop->link;
	//}

	if(loop)
	{
		CDownloadManager *tmp = (CDownloadManager*)loop->info;
		if (tmp)
		{
			tmp->EnableCallBack(FALSE);
			tmp->StopDownload();
		}
	}	

	m_csCntl.Unlock();
	m_bQuery = TRUE;
	return TRUE;
}
//��������
BOOL CTaskManager::MoveOnTask(int nPos)
{
	//if( nPos < 1 || nPos > GetTaskNum() )
	//{
	//	// ѡ������񳬳���Χ
	//	return FALSE;
	//}

	BOOL nRet = FALSE;
	//��ͣquery�Ĳ��� 
	m_bQuery = FALSE;
	m_csCntl.Lock();

	NODE *loop = m_pHead->link;
	while(loop != NULL)
	{
		CDownloadManager *tmp = (CDownloadManager*)loop->info;
		if(tmp->GetSerialNum() == nPos)
		{
			break;
		}
		loop = loop->link;
	}
	//NODE *loop = m_pHead;
	//for( int i = 0; i < nPos; i++ )
	//{
	//	loop = loop->link;
	//}

	if(loop)
	{
		CDownloadManager *tmp = (CDownloadManager*)loop->info;
		if (tmp)
		{
			tmp->EnableCallBack();
			//if(!tmp->IsComplete())
			//{
				int nCreate = tmp->CreatEndEvent();
				if(nCreate == 0)
				{
					//�����Ч��ԭ�������ڽ���
					nRet = TRUE;
				}
				else if(nCreate < 0)
				{
					nRet = FALSE;
				}
				else
				{
					if(tmp->Download(loop->szUrl, loop->szSavePath, loop->szSaveFileName, nPos))
					{
						nRet = TRUE;
					}
				}
			//}
			//else
			//{
			//	// �����
			//	nRet = TRUE;
			//}
		}
	}

	m_csCntl.Unlock();
	m_bQuery = TRUE;
	return nRet;
}

int CTaskManager::GetNodeInfo(int nPos, TCHAR *szUrl, TCHAR *szSavePath, TCHAR *szSaveFileName)
{
	NODE *loop = m_pHead->link;
	while(loop != NULL)
	{
		CDownloadManager *tmp = (CDownloadManager*)loop->info;
		if(tmp->GetSerialNum() == nPos)
		{
			break;
		}
		loop = loop->link;
	}
	if(loop != NULL)
	{
		if (szUrl&&loop->szUrl)
		{
			wcscpy(szUrl, loop->szUrl);
		}

		if (szSavePath&&loop->szSavePath)
		{
			wcscpy(szSavePath, loop->szSavePath);
		}

		if (szSaveFileName&&loop->szSaveFileName)
		{
			wcscpy(szSaveFileName, loop->szSaveFileName);
		}
	}

	return 0;
}

bool CTaskManager::SetObserver( IObserver *pObserver )
{
	m_pObserver = pObserver;
	return true;
}