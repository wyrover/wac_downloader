#include "StdAfx.h"
#include "DownloaderManager.h"
#include "TaskManager.h"

namespace ns_WAC
{
	//IDownloaderManager* DownloaderManager::s_sinalton = NULL;

	DownloaderManager::DownloaderManager(void)
	{
		m_pDownloadCntl = NULL;
		m_hCallbackWnd = NULL;
		m_pcb = NULL;
		m_icb = NULL;
		m_nCallBackMsgID = 0;

		// log
#ifdef __LOG_ENABLE__
		{
			CString szFilePath = _T("");
			//日志存放在当前路径
			wchar_t lpFilePath[MAX_PATH] = {0};
			::GetModuleFileName(NULL, lpFilePath, MAX_PATH );
			szFilePath = lpFilePath;	
			szFilePath = szFilePath.Left(szFilePath.ReverseFind(_T('\\'))+1);
			CString szLogFilePathName = szFilePath + LOG_FUNCTION_FILE_NAME;

			BOOL bLog = FALSE;
			Logger *pLog = new Logger(szLogFilePathName.GetBuffer(szLogFilePathName.GetLength()));
			if(pLog)
			{
				if(pLog->IsOK())
				{
					bLog = TRUE;
					FileLogger::addAppender(LOG_FUNCTION_APPENDER_NAME, pLog);
				}
			}
			if(!bLog)
			{
				// function log init failed
			}
		}
#endif
		__LOG_LOG__( _T("Function init suceessful."), L_LOGSECTION_APP_INIT, L_LOGLEVEL_TRACE);


		if (!m_pDownloadCntl)
		{
			//初始化下载管理类指针
			m_pDownloadCntl = new CTaskManager;
			m_pDownloadCntl->SetObserver(this);
		}
	}


	DownloaderManager::~DownloaderManager(void)
	{
		ReleaseCallBack();
		//释放下载管理类指针
		if (m_pDownloadCntl)
		{
			if (m_pDownloadCntl->GetTaskNum() > 0)
			{
				//先清理下载任务
				Clear();
			}
			delete m_pDownloadCntl;
			m_pDownloadCntl = NULL;
		}

		__LOG_LOG__( _T("Function uninit suceessful."), L_LOGSECTION_APP_UNINIT, L_LOGLEVEL_TRACE);
		FileLogger::removeAppenders(LOG_FUNCTION_APPENDER_NAME);
	}

	//IDownloaderManager* DownloaderManager::get_instance()
	//{
	//	if(s_sinalton == NULL)
	//	{
	//		s_sinalton = new DownloaderManager;
	//	}
	//	return s_sinalton;
	//}

	BOOL DownloaderManager::Cancel(int nTaskID)
	{
		if( NULL == m_pDownloadCntl )
		{
			return FALSE;
		}
		return m_pDownloadCntl->RemoveTheTask(nTaskID);
	}

	void DownloaderManager::SetCallBack(IDownloaderManagerCallBack* pcb)
	{
		m_pcb = pcb;
		ReleaseCallBack();

		m_hCallbackWnd = NULL;
		m_nCallBackMsgID = 0;
	}

	void DownloaderManager::SetCallBack(ICallBack* pcb)
	{
		m_icb = pcb;
		if(m_icb != NULL)
		{
			m_icb->AddRef();
		}

		m_pcb = NULL;
		CString szLog;
		szLog.Format(_T("set callback succ."));
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);
		m_hCallbackWnd = NULL;
		m_nCallBackMsgID = 0;
	}

	void DownloaderManager::ReleaseCallBack()
	{
		if(m_icb != NULL)
		{
			m_icb->Release();
			m_icb = NULL;
		}
	}

	void DownloaderManager::SetCallBack(HWND hWnd, UINT nMsgID)
	{
		m_hCallbackWnd = hWnd;
		m_nCallBackMsgID = nMsgID;

		m_pcb = NULL;
		ReleaseCallBack();
	}

	unsigned DownloaderManager::GetTaskCount()
	{
		if( NULL == m_pDownloadCntl )
		{
			return 0;
		}
		return m_pDownloadCntl->GetTaskNum();
	}

	//BOOL DownloaderManager::GetAllTask(__out DownloadTask *pDownload, int nBufferSize)
	//{
	//	//如果size不对
	//	if (nBufferSize <= 0)
	//	{
	//		return FALSE;
	//	}

	//	if( NULL == m_pDownloadCntl )
	//	{
	//		return FALSE;
	//	}

	//	int nTaskTotal = m_pDownloadCntl->GetTaskNum();
	//	if (nTaskTotal <= 0)
	//	{
	//		return FALSE;
	//	}
	//	//传入的nBufferSize大小跟任务数不一致则返回
	//	else if (nBufferSize / sizeof(DownloadTask) != nTaskTotal)
	//	{
	//		return FALSE;
	//	}

	//	//按任务数量遍历
	//	for (int i=0;i<nTaskTotal;i++)
	//	{
	//		if (!QueryTask(i, pDownload))
	//		{
	//			return FALSE;
	//		}
	//		pDownload++;
	//	}
	//	return TRUE;
	//}

	BOOL DownloaderManager::Clear()
	{
		if( NULL == m_pDownloadCntl )
		{
			return FALSE;
		}
		return m_pDownloadCntl->RemoveAllTask();
	}
	BOOL DownloaderManager::Pause(int nTaskID)
	{
		if( NULL == m_pDownloadCntl )
		{
			return FALSE;
		}
		return m_pDownloadCntl->StopTask(nTaskID);
	}

	BOOL DownloaderManager::Continue(int nTaskID)
	{
		if( NULL == m_pDownloadCntl )
		{
			return FALSE;
		}
		return m_pDownloadCntl->MoveOnTask(nTaskID);
	}

	int DownloaderManager::AddTask(const wchar_t *pszUrl,
									 const wchar_t *pszSavePath,		
									 const wchar_t *pszSaveFileName,	
									 int  PID)	
	{
		if(m_pDownloadCntl == NULL || (pszUrl == NULL && PID <= 0))
		{
			return -1;
		}
		return m_pDownloadCntl->AddTask( pszUrl, pszSavePath, pszSaveFileName, PID);

		////下载地址
		//wchar_t pchUrl[1024] = {0};
		////保存路径
		//wchar_t pchSavePath[1024] = {0};
		////保存文件名
		//wchar_t pchSaveFileName[1024] = {0};
		//if(pszUrl != NULL)
		//{
		//	wcscpy(pchUrl, pszUrl);
		//}
		//

		//if(!pszSavePath)
		//{
		//	wchar_t lpPathBuffer[2 * MAX_PATH] = {0};
		//	DWORD dwRetVal = ::GetTempPath(sizeof(lpPathBuffer), lpPathBuffer);
		//	if ((dwRetVal > sizeof(lpPathBuffer)) || (dwRetVal == 0))
		//	{
		//		return -1;
		//	}
		//	wcscpy_s(pchSavePath, _countof(pchSavePath), lpPathBuffer);
		//}
		//else
		//{
		//	wcscpy_s(pchSavePath, _countof(pchSavePath), pszSavePath);
		//}

		//if(!pszSaveFileName)
		//{
		//	// 使用原本的文件名，不使用指定文件名
		//}
		//else
		//{
		//	wcscpy(pchSaveFileName, pszSaveFileName);
		//}

		//return nID;
	}

	BOOL DownloaderManager::RemoveTask(int nTaskID)
	{
		if( NULL == m_pDownloadCntl )
		{
			return FALSE;
		}
		return m_pDownloadCntl->RemoveTheTask(nTaskID);
	}

	BOOL DownloaderManager::QueryTask(int nTaskID, DownloadTask* sDownloadTask)
	{
		__int64 nTotalDownloadedSizeThisTime = 0;
		int nElapsedTime = 0;
		if(!m_pDownloadCntl->QueryState(nTaskID, sDownloadTask->cbTotle, sDownloadTask->cbCurPos, nTotalDownloadedSizeThisTime, nElapsedTime))
		{
			return FALSE;
		}

		if ((sDownloadTask->cbTotle > 0) && (sDownloadTask->cbCurPos >= 0))
		{
			if(nElapsedTime < 1000)
			{
				sDownloadTask->cbSpeedPerSecond = 0;
			}
			else
			{
				sDownloadTask->cbSpeedPerSecond = int(nTotalDownloadedSizeThisTime / ((nElapsedTime / 1000)));
			}

			m_pDownloadCntl->GetNodeInfo(nTaskID, sDownloadTask->szUrl, sDownloadTask->szSavePath, sDownloadTask->szSaveFileName);
			sDownloadTask->nID = nTaskID;
		}
		else
		{
			sDownloadTask->cbTotle = -1;
			sDownloadTask->cbCurPos = -1;
			sDownloadTask->nID = nTaskID;
		}

		/*
		//如果query成功
		if (sDownloadTask.cbTotle>0&&sDownloadTask.cbCurPos>0&&nTotalDownloadedSizeThisTime>0&&nElapsedTime>0)
		{
		if ( nElapsedTime > 2*1000 || ( nElapsedTime > 1000 && nTotalDownloadedSizeThisTime > 1024 ) )
		{
		dSpeed = nTotalDownloadedSizeThisTime / ((nElapsedTime / 1000));
		sDownloadTask.cbSpeedPerSecond = dSpeed;
		}

		m_pDownloadCntl->GetNodeInfo(nTaskID, sDownloadTask.szUrl, sDownloadTask.szSavePath, sDownloadTask.szSaveFileName);
		sDownloadTask.nID = nTaskID;
		}
		//否则返回false
		else
		{
		return FALSE;
		}
		*/
		return TRUE;
	}


	void DownloaderManager::OnNotify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam)
	{
		if (!pdlntfData)
		{
			return;
		}

		if(m_hCallbackWnd && (m_nCallBackMsgID != 0))
		{
			//::SendMessage(m_hCallbackWnd, m_nCallBackMsgID, wParam, (LPARAM)pdlntfData );
			//修改发送消息方式，防止堵塞
			DWORD_PTR result;
			LRESULT rv = ::SendMessageTimeout(m_hCallbackWnd, m_nCallBackMsgID, wParam, (LPARAM)pdlntfData, 
								SMTO_NORMAL, 300, &result);
			if(rv == 0 && (GetLastError() == ERROR_TIMEOUT|| GetLastError() == ERROR_INVALID_PARAMETER))
			{
				CString szLog;
				szLog.Format(_T("SendMessageTimeout."));
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);
			}
			return;
		}

		if(m_pcb)
		{
			CString csStatus;
			int nSerialNum = -1;
			switch ( pdlntfData->nNotityType )
			{
				//获得文件名字
			case NOTIFY_TYPE_HAS_GOT_REMOTE_FILENAME:
				{
					CString szFileName((wchar_t*)pdlntfData->lpNotifyData);
					m_pcb->OnGetFileName(pdlntfData->nSerialNum, szFileName.GetBuffer(szFileName.GetLength())); 
					break;
				}
				//获得服务器端文件大小
			case NOTIFY_TYPE_HAS_GOT_REMOTE_FILESIZE:
				{
					m_pcb->OnGetFileSize(pdlntfData->nSerialNum, (__int64)pdlntfData->lpNotifyData);
					break;
				}
				//下载结束
			case NOTIFY_TYPE_HAS_END_DOWNLOAD:
				{
					NsResultEnum eDownloadResult = (NsResultEnum)(int)pdlntfData->lpNotifyData;
					if ( eDownloadResult == RS_SUCCESS)
					{
						//下载正常并且成功
						m_pcb->OnComplete(pdlntfData->nSerialNum);
					}
					else
					{
						//下载失败
						m_pcb->OnFail(pdlntfData->nSerialNum, eDownloadResult);
					}
					break;
				}
				//下载过程信息
			case NOTIFY_TYPE_HAS_GOT_NEW_DOWNLOADED_DATA:
				{
					NotifyData *pInfo = (NotifyData*)pdlntfData->lpNotifyData;
					m_pcb->OnUpdate(pdlntfData->nSerialNum, pInfo->cbCurPos, pInfo->cbTotle, pInfo->cbSpeedPerSecond);
					delete pInfo;
					pInfo = NULL;

					break;
				}
				// 获得某线程将要下载的文件大小
			case NOTIFY_TYPE_HAS_GOT_THREAD_WILL_DOWNLOAD_SIZE:
				{
					m_pcb->OnTest(pdlntfData->nSerialNum, pdlntfData->nIndex, 0, (__int64)pdlntfData->lpNotifyData);
					break;
				}
				// 获得某线程已经下载的文件大小
			case NOTIFY_TYPE_HAS_GOT_THREAD_DOWNLOADED_SIZE:
				{
					m_pcb->OnTest(pdlntfData->nSerialNum, pdlntfData->nIndex, 1, (__int64)pdlntfData->lpNotifyData);
					break;
				}
				// 获得某线程已经下载的缓存数据大小
			case NOTIFY_TYPE_HAS_GOT_THREAD_CACHE_SIZE:
				{
					m_pcb->OnTest(pdlntfData->nSerialNum, pdlntfData->nIndex, 2, (__int64)pdlntfData->lpNotifyData);
					break;
				}
			case NOTIFY_TYPE_HAS_START:
				{
					m_pcb->onStart(pdlntfData->nSerialNum);
					break;
				}
			default:
				break;
			}
		}

		if(m_icb)
		{
			//CString szLog;
			//szLog.Format(_T("do callback."));
			//__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);

			CString csStatus;
			int nSerialNum = -1;
			switch ( pdlntfData->nNotityType )
			{
				//获得文件名字
			case NOTIFY_TYPE_HAS_GOT_REMOTE_FILENAME:
				{
					CString szFileName((wchar_t*)pdlntfData->lpNotifyData);
					m_icb->OnGetFileName(pdlntfData->nSerialNum, szFileName.GetBuffer(szFileName.GetLength())); 
					break;
				}
				//获得服务器端文件大小
			case NOTIFY_TYPE_HAS_GOT_REMOTE_FILESIZE:
				{
					m_icb->OnGetFileSize(pdlntfData->nSerialNum, (__int64)pdlntfData->lpNotifyData);
					break;
				}
				//下载结束
			case NOTIFY_TYPE_HAS_END_DOWNLOAD:
				{
					NsResultEnum eDownloadResult = (NsResultEnum)(int)pdlntfData->lpNotifyData;
					if ( eDownloadResult == RS_SUCCESS)
					{
						//下载正常并且成功
						m_icb->OnComplete(pdlntfData->nSerialNum);
					}
					else
					{
						//下载失败
						m_icb->OnFail(pdlntfData->nSerialNum, eDownloadResult);
					}
					break;
				}
				//下载过程信息
			case NOTIFY_TYPE_HAS_GOT_NEW_DOWNLOADED_DATA:
				{
					NotifyData *pInfo = (NotifyData*)pdlntfData->lpNotifyData;

					m_icb->OnUpdate(pdlntfData->nSerialNum, pInfo->cbCurPos, pInfo->cbTotle, pInfo->cbSpeedPerSecond);
					delete pInfo;
					pInfo = NULL;

					break;
				}
			case NOTIFY_TYPE_HAS_START:
				{
					m_icb->OnStart(pdlntfData->nSerialNum);
					break;
				}
			default:
				break;
			}
		}
	}
}