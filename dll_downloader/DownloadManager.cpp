/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:26
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\DownloadManager.cpp
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	DownloadManager
	file ext:	cpp
	author:		tanyc
	
	purpose:	
*********************************************************************/
#include "stdafx.h"
#include "DownloadManager.h"
#include "MD5.h"
#include <io.h>
#include "../../Common/Markup.h"
#include "ChttpRequest.h"
#include "../../Common/util.h"
#include "../../Common/HttpRequest.h"
#include "../../Common/server_interface_url.h"

CDownloadManager::CDownloadManager()
: m_nThreadCount ( DEFAULT_THREAD_COUNT )
, m_pDownloadPub_MTR ( NULL )
, m_pDownloadPub_Info ( NULL )
, m_pDownloadCellInfo ( NULL )
, m_hThread ( NULL )
, m_bForceDownload ( FALSE )
, m_nTotalDownloadedSize_ThisTimes ( 0 )
{
	memset ( &m_BaseDownInfo, 0, sizeof(t_BaseDownInfo) );
	m_hEvtEndModule = ::CreateEvent ( NULL, TRUE, FALSE, NULL );
	m_dwDownloadStartTime = GetTickCount();
	m_nSerialNum = -1;
	m_bNeedCheckMD5 = FALSE;
	m_pObserver = NULL;
	SetComplete(FALSE);
	EnableCallBack();

	m_last_report_time = GetTickCount();
	srand( (unsigned)time( NULL ) );
}

CDownloadManager::~CDownloadManager()
{
	StopDownload ();
	CCustomizeSocket::ClearHost();
}


//�������ص��߳���

BOOL CDownloadManager::SetThreadCount(int nThreadCount)
{
	if ( nThreadCount <= 0 || nThreadCount > MAX_DOWNLOAD_THREAD_COUNT )
	{
		CString szLog;
		szLog.Format(_T("Set thread count: %d is invalid. The valid rang is [%d-%d]."), nThreadCount, 1, MAX_DOWNLOAD_THREAD_COUNT);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
		return FALSE;
	}
	if ( nThreadCount == m_nThreadCount )
		return TRUE;

	m_nThreadCount = nThreadCount;
	return TRUE;
}

//
// ����������̺߳���
//
DWORD WINAPI ThreadProc_DownloadMTR(LPVOID lpParameter)
{
	CDownloadManager *pDownloadMTR = (CDownloadManager*)lpParameter;
	_ASSERT ( pDownloadMTR );
	pDownloadMTR->ThreadProc_DownloadMTR ();

	CLOSE_HANDLE ( pDownloadMTR->m_hEvtEndModule );

	CString szLog = _T("The main download thread quit.");
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	return TRUE;
}

BOOL CDownloadManager::ThreadProc_DownloadMTR()
{

	//////////////////////////////////////////////////////////////////////////
	//���Ϳ�ʼ���ص���Ϣ
	if(IsCallBackEnable())
	{
		t_DownloadNotifyPara DownloadNotifyPara = {0};
		DownloadNotifyPara.nIndex = -1;
		DownloadNotifyPara.nNotityType = NOTIFY_TYPE_HAS_START;
		DownloadNotifyPara.lpNotifyData = NULL;
		DownloadNotifyPara.nSerialNum = GetSerialNum();
		if(m_pObserver != NULL)
		{
			m_pObserver->OnNotify(&DownloadNotifyPara, NULL);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//ʹ��PID��ȡ��Ʒ��Ϣ
	if(m_PID > 0)
	{
		NsResultEnum nRet = RequestPIDInfo(m_csDownloadURL);
		if(nRet != RS_SUCCESS)
		{
			CString szLog;
			szLog.Format(_T("RequestPIDInfo return: %d."), nRet);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
			return HandleDownloadFinished(nRet);
		}
	}



	//���������߳�������������Ҫ����
	if(m_pid_info.part_infos_.size() > 0)
	{
		SetThreadCount(m_pid_info.part_infos_.size());
		CString szLog;
		szLog.Format(_T("SetThreadCount: %d with pid_info"), m_pid_info.part_infos_.size());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	}
	else
	{
		SetThreadCount(DEFAULT_THREAD_COUNT);
		CString szLog;
		szLog.Format(_T("SetThreadCount: %d with default"), DEFAULT_THREAD_COUNT);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	}
	if(m_pid_info.md5_ != _T(""))
	{
		m_bNeedCheckMD5 = TRUE;
		m_csMD5_Web = m_pid_info.md5_.c_str();
	}

	//������Դ���
	for(int i = 0; i < RETRY_TIME; ++i)
	{
	// �������߳���������
		CString szLog;
		szLog.Format(_T("StartMTRDownload retry time: %d, save url: %s"), i, m_csDownloadURL);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

		NsResultEnum nRet = StartMTRDownload ();
		if(nRet != RS_SUCCESS)
		{
			CString szLog;
			szLog.Format(_T("StartMTRDownload return error: %d"), nRet);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

			if(nRet == RS_NO_NEED_DOWNLOAD)
			{
				SetComplete(TRUE);
				if(IsCallBackEnable())
				{
					Notify ( -1, GetSerialNum(), NOTIFY_TYPE_HAS_END_DOWNLOAD, (LPVOID)RS_SUCCESS );
				}
				return TRUE;
			}
			else
			{
				BOOL rv = HandleDownloadFinished(nRet);
				if(rv == RS_ERR_USER_CANCEL || rv == RS_ERR_DISK_SPACE_NOT_ENOUGH)
				{
					return FALSE;
				}
			}
		}

		NsResultEnum eDownloadResult = nRet;
		if(eDownloadResult == RS_SUCCESS)
		{
			// �ȴ������߳��������
			eDownloadResult = WaitForDownloadFinished();
			if(eDownloadResult == RS_SUCCESS)
			{
				if(GetDownloadResult())
				{
					SetComplete(TRUE);
				}
				else
				{
					eDownloadResult = RS_ERR_DOWNLOAD_FAILED;
				}
			}
			BOOL rv = HandleDownloadFinished ( eDownloadResult );
			if(rv == RS_ERR_USER_CANCEL)
			{
				return FALSE;
			}
		}
	
		//����
		//if(i == 0)
		//{
		//	eDownloadResult = RS_ERR_MD5_CHECK_FAILED;
		//}
		if(eDownloadResult == RS_ERR_FILE_SIZE_INCORRECT || eDownloadResult == RS_ERR_MD5_CHECK_FAILED)
		{
			CString szLog;
			szLog.Format(_T("delete file for error: %d"), eDownloadResult);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			::DeleteFile(m_csSavePathFileName);
			if(!m_csSavePathFileName.IsEmpty())
			{
				::DeleteFile(GetTempFilePath());
			}
		}
		
		DeleteDownloadObjectAndDataMTR ();

		if(i == RETRY_TIME - 1 || eDownloadResult == RS_SUCCESS)
		{
			if(IsCallBackEnable())
			{
				int nSerialNum = GetSerialNum();
				Notify ( -1, nSerialNum, NOTIFY_TYPE_HAS_END_DOWNLOAD, (LPVOID)eDownloadResult );
			}
			return TRUE;
		}

		//˯��N�������
		int sleep_time = 3 + 5 * i;
		for(int count = 0; count < sleep_time; ++count)
		{
			if(::WaitForSingleObject ( m_hEvtEndModule, 100 ) == WAIT_OBJECT_0 )
			{
				return FALSE;
			}
			Sleep(1000);
		}
	}
	
	return TRUE;
}

NsResultEnum CDownloadManager::RequestPIDInfo(LPCTSTR lpszDownloadURL)
{
	//std::string url = "http://10.10.24.83/down_info_";
	//char pidBuf[10];
	//memset(pidBuf, 0, sizeof(pidBuf));
	//itoa(m_PID, pidBuf, 10);
	//url += pidBuf;
	//url += ".xml";
	//std::string url = DWN_INFO_URL;
	//url += "?client_sign=";
	//std::wstring client_sign = GetClientSign();
	//char sign_buf[128];
	//memset(sign_buf, 0, sizeof(sign_buf));
	//wcstombs(sign_buf, client_sign.c_str(), client_sign.size());
	//url += sign_buf;

	//url += "&downurl=";
	//char downurl_buf[1024];
	//memset(downurl_buf, 0, sizeof(downurl_buf));
	//wcstombs(downurl_buf, lpszDownloadURL, lstrlen(lpszDownloadURL));
	//url += downurl_buf;

	//CString szLog;
	//szLog.Format(_T("GetClientSign: %s"), GetClientSign().c_str());
	//__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	std::string server_url = ServerInterfaceUrl::GetInstance()->DownInfoUrl;
	std::wstring url = s2ws(server_url);
	//std::wstring url = _T(DWN_INFO_URL);
	url += _T("?client_sign=");
	url += GetClientSign();
	url += _T("&download_url=");
	url += lpszDownloadURL;

	wchar_t content[MAX_RESP_BUF];
	memset(content, 0, _countof(content));
	int ret_size = _countof(content);

	ChttpRequest hRequest;
	if(hRequest.Get(url, content, ret_size) != 0)
	//if(http_get(url.c_str(), content, ret_size) != 0)
	{
		//wchar_t temp_url[MAX_URL_LENGTH];
		//memset(temp_url, 0, _countof(temp_url));
		//mbstowcs(temp_url, url.c_str(), url.size());

		CString szLog;
		szLog.Format(_T("http_get failed, url: %s"), url.c_str());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return RS_SUCCESS;
	}

	CString szLog;
	szLog.Format(_T("http_get succ, url: %s\ncontent: %s"), url.c_str(), content);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	//wchar_t wcontent[MAX_RESP_BUF];
	//memset(wcontent, 0, MAX_RESP_BUF);
	//mbstowcs(wcontent, content, ret_size);

	MCD_STR str(content);

	CMarkup xml(str);

	if(xml.FindElem(_T("wsrp")))
	{
		if(xml.GetAttrib(_T("status")) != _T("ok"))
		{
			return RS_SUCCESS;
		}
		if(xml.FindChildElem(_T("servers")))
		{
			xml.IntoElem();
			while (xml.FindChildElem(_T("link")))
			{
				xml.IntoElem();
				ServerHost serHost;
				serHost.server_ = xml.GetAttrib(_T("host"));
				serHost.weight_ = _wtoi(xml.GetAttrib(_T("weight")).c_str());
				m_pid_info.servers_.push_back(serHost);
				xml.OutOfElem();
			}
			xml.OutOfElem();
		}

		xml.ResetChildPos();

		if(xml.FindChildElem(_T("md5_link")))
		{
			xml.IntoElem();
			m_pid_info.md5_url_ = xml.GetAttrib(_T("href"));
			xml.OutOfElem();
		}

		xml.ResetChildPos();

		if(xml.FindChildElem(_T("download_url")))
		{
			xml.IntoElem();
			m_pid_info.downurl_ = xml.GetAttrib(_T("href"));
			xml.OutOfElem();
		}
	}

	if ( ::WaitForSingleObject ( m_hEvtEndModule, 100 ) == WAIT_OBJECT_0 )
	{
		szLog = _T("User canceled after get the remotesite info.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return RS_ERR_USER_CANCEL;
	}

	if(!m_pid_info.md5_url_.empty())
	{
		return getMd5Info(lpszDownloadURL);
	}
	return RS_SUCCESS;
}

BOOL CDownloadManager::CheckLastModifyTime(LPCTSTR exe_url, LPCTSTR xml_url)
{
	ChttpRequest hRequest;
	CTime ExeTimeLastModified;
	BOOL rv = hRequest.GetModifyTime(exe_url, ExeTimeLastModified);
	if(!rv)
	{
		return FALSE;
	}

	if ( ::WaitForSingleObject ( m_hEvtEndModule, 100 ) == WAIT_OBJECT_0 )
	{
		CString szLog;
		szLog = _T("User canceled after get the remotesite info.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return RS_ERR_USER_CANCEL;
	}
	CTime XmlTimeLastModified;
	rv = hRequest.GetModifyTime(xml_url, XmlTimeLastModified);
	if(!rv)
	{
		return FALSE;
	}

	if(ExeTimeLastModified <= XmlTimeLastModified)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

 NsResultEnum CDownloadManager::getMd5Info(LPCTSTR lpszDownloadURL)
{
	BOOL rv = CheckLastModifyTime(lpszDownloadURL, m_pid_info.md5_url_.c_str());
	if(rv != TRUE)
	{
		CString szLog;
		szLog.Format(_T("CheckLastModifyTime, DownloadURL: %s newer than xml_url: %s, rv: %d"), lpszDownloadURL, m_pid_info.md5_url_.c_str(), rv);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		if(rv == RS_ERR_USER_CANCEL)
		{
			return RS_ERR_USER_CANCEL;
		}
		else
		{
			return RS_SUCCESS;
		}
	}

	//char url[MAX_URL_LENGTH];
	//memset(url, 0, MAX_URL_LENGTH);

	//wcstombs(url, m_pid_info.md5_url_.c_str(), m_pid_info.md5_url_.size());

	//char content[MAX_RESP_BUF];
	//memset(content, 0, MAX_RESP_BUF);
	//size_t ret_size = MAX_RESP_BUF;

	wchar_t content[MAX_RESP_BUF];
	memset(content, 0, _countof(content));
	int ret_size = _countof(content);
	
	ChttpRequest hRequest;
	if(hRequest.Get(m_pid_info.md5_url_, content, ret_size) != 0)
	//if(http_get(url, content, ret_size) != 0)
	{
		CString szLog;
		szLog.Format(_T("http_get failed, url: %s"), m_pid_info.md5_url_.c_str());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return RS_SUCCESS;
	}

	CString szLog;
	szLog.Format(_T("http_get succ, url: %s\ncontent: %s"), m_pid_info.md5_url_.c_str(), content);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	//wchar_t wcontent[MAX_RESP_BUF];
	//memset(wcontent, 0, MAX_RESP_BUF);
	//mbstowcs(wcontent, content, ret_size);

	MCD_STR str(content);

	CMarkup xml(str);

	if(xml.FindElem(_T("wsrp")))
	{
		std::wstring status = xml.GetAttrib(_T("status"));
		if(status != _T("ok"))
		{
			return RS_SUCCESS;
		}
		if(xml.FindChildElem(_T("parts")))
		{
			xml.IntoElem();
			while (xml.FindChildElem(_T("part")))
			{
				xml.IntoElem();
				PartInfo part_info;
				part_info.offset_ = _wtol(xml.GetAttrib(_T("offset")).c_str());
				part_info.length_ = _wtol(xml.GetAttrib(_T("length")).c_str());
				part_info.md5_ = xml.GetAttrib(_T("MD5"));
				m_pid_info.part_infos_.push_back(part_info);
				xml.OutOfElem();
			}
			xml.OutOfElem();
		}
	}
	return RS_SUCCESS;
}
//
// ���̶߳ϵ���������һ���ļ�
//
BOOL CDownloadManager::Download (
							 LPCTSTR lpszDownloadURL,
							 LPCTSTR lpszSavePath,
							 LPCTSTR lpszSaveOnlyFileName,
							 int iPID,
							 LPCTSTR lpszUsername/*=NULL*/,
							 LPCTSTR lpszPassword/*=NULL*/,
							 BOOL bForceDownload/*=FALSE*/		// ���Ϊ TRUE ��ʾǿ�����������أ������صĲ��ֽ��ᱻɾ����FALSE ��ʾ�ϵ�����
							 )
{
	if ( !HANDLE_IS_VALID(m_hEvtEndModule) )
	{
		CLOSE_HANDLE ( m_hEvtEndModule );
		return FALSE;
	}
	if ( !lpszSavePath || wcslen(lpszSavePath) < 1 )
	{
		CLOSE_HANDLE ( m_hEvtEndModule );
		return FALSE;
	}

	if(lpszSavePath == NULL || iPID <= 0)
	{
		CLOSE_HANDLE(m_hEvtEndModule);
		return FALSE;
	}

	m_PID = iPID;
	CString downurl = lpszDownloadURL;
	if(downurl == _T(""))
	{
		__LOG_LOG__( _T("Download url empty."), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return FALSE;
	}

	CString szLog;
	szLog.Format(_T("Download url(%s), save path(%s), file name(%s)."), downurl.GetBuffer(downurl.GetLength()), lpszSavePath, lpszSaveOnlyFileName);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	m_csSavePath = lpszSavePath;
	m_csSaveOnlyFileName = GET_SAFE_STRING(lpszSaveOnlyFileName);
	m_bForceDownload = bForceDownload;

	//host and path
	CString csServer, csObject;
	USHORT nPort = 0;

	// �û��ṩ��URL�����Ѿ�����ת�崦��EnCode����ҪDeCode��ԭΪԭ����URL
	wchar_t c_decode_url[1024] = {0};
	URL_Decode(downurl.GetBuffer(downurl.GetLength()), c_decode_url);

	szLog.Format(_T("url(%s) decode : (%s)."), downurl.GetBuffer(downurl.GetLength()), c_decode_url);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);


	if ( !ParseURLDIY ( c_decode_url, csServer, csObject, nPort, m_csProtocolType ) )
	{
		CLOSE_HANDLE ( m_hEvtEndModule );

		szLog.Format(_T("Analyze download url failed [%s]."), c_decode_url);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
		return FALSE;
	}
	m_csDownloadURL = c_decode_url;

	// ����ȡվ����Ϣ����
	if ( !( m_pDownloadPub_Info = CreateDownloadObject (1) ) )
	{
		CLOSE_HANDLE ( m_hEvtEndModule );

		szLog = _T("Create download object failed.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
		NsResultEnum rv = RS_ERR_GET_WEBINFO_FAILED;
		return HandleDownloadFinished(rv);
	}

	m_pDownloadPub_Info->SetTaskID(m_nSerialNum);

	// ����ȡվ����Ϣ����Ĳ���
	m_pDownloadPub_Info->SetAuthorization ( lpszUsername, lpszPassword );
	m_pDownloadPub_Info->m_pDownloadMTR = this;
	m_pDownloadPub_Info->SetDownloadUrl ( c_decode_url );
	m_pDownloadPub_Info->SetObserver(m_pObserver);

	// ����һ�������߳�
	DWORD dwThreadId = 0;
	m_hThread = CreateThread ( NULL, 0, ::ThreadProc_DownloadMTR, LPVOID(this), 0, &dwThreadId );
	if ( !HANDLE_IS_VALID(m_hThread) )
	{
		CLOSE_HANDLE ( m_hEvtEndModule );

		szLog = _T("Create main download thread failed.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
		return FALSE;
	}
	return TRUE;
}

//
// �������ض���
//
CDownloadBase* CDownloadManager::CreateDownloadObject ( int nCount/*=1*/ )
{
	if ( nCount < 1 ) return NULL;
	CDownloadBase *pDownloadPub = NULL;
	if ( m_csProtocolType.CompareNoCase ( _T("http") ) == 0 )
	{
		pDownloadPub = (CDownloadBase*)new CDownloadHttp[nCount];
	}
	//ftp��ʱ��֧��
	//else if ( m_csProtocolType.CompareNoCase ( _T("ftp") ) == 0 )
	//{
	//	pDownloadPub = (CDownloadBase*)new CDownloadFtp[nCount];
	//}
	else return NULL;

	return pDownloadPub;
}

//
// ɾ�����ض���
//
void CDownloadManager::DeleteDownloadObject ( CDownloadBase *pDownloadPub )
{
	if ( m_csProtocolType.CompareNoCase ( _T("http") ) == 0 )
	{
		delete[] ( (CDownloadHttp*)pDownloadPub );
	}
	else if ( m_csProtocolType.CompareNoCase ( _T("ftp") ) == 0 )
	{
		delete[] ( (CDownloadFtp*)pDownloadPub );
	}
	else delete[] pDownloadPub;
}

void Callback_SaveDownloadInfo ( int nIndex, __int64 nDownloadedSize, __int64 nSimpleSaveSize, WPARAM wParam )
{
	CDownloadManager *pDownloadMTR = (CDownloadManager*)wParam;
	_ASSERT ( pDownloadMTR );
	pDownloadMTR->Callback_SaveDownloadInfo ( nIndex, nDownloadedSize, nSimpleSaveSize );
}

// nIndex : �߳����
// nDownloadedSize : ��ǰ�̣߳��ۼ��Ѿ����浽�ļ����ֽ���
// nSimpleSaveSize : ��ǰ�̣߳��˴α��浽�ļ����ֽ���
void CDownloadManager::Callback_SaveDownloadInfo ( int nIndex, __int64 nDownloadedSize, __int64 nSimpleSaveSize )
{
	if ( nIndex >= 0 && nIndex < m_nThreadCount )
	{
		if ( nDownloadedSize >= 0 )
		{
			if(nDownloadedSize > 0)
			{
				m_pDownloadCellInfo[nIndex].nDownloadedSize = nDownloadedSize;
			}
			
			__int64 nTotalDownloadedSize_ThisTimes = 0;
			m_CSFor_DownloadedData.Lock();
			m_nTotalDownloadedSize_ThisTimes += nSimpleSaveSize;
			nTotalDownloadedSize_ThisTimes = m_nTotalDownloadedSize_ThisTimes;
			m_CSFor_DownloadedData.Unlock ();

			//�ص�
			if(IsCallBackEnable())
			{
				Notify ( nIndex, GetSerialNum(), NOTIFY_TYPE_HAS_GOT_THREAD_DOWNLOADED_SIZE, (LPVOID)nDownloadedSize );

				if ( m_pDownloadPub_Info )
				{
					__int64 nTotalUndownloadBytes = 0;
					for ( int nIndex = 0; nIndex < m_nThreadCount; nIndex++ )
					{
						// ע�⣺��ʱ���������ֽ���Ϊ0�� Get_TempSaveBytes�������ܻ�ȡ����ȷ�Ļ������ֽ�������Ϊ�ص���Ÿ��´�����
						nTotalUndownloadBytes += (m_pDownloadPub_MTR[nIndex].Get_WillDownloadSize () - ( m_pDownloadPub_MTR[nIndex].Get_DownloadedSize () + 0 ));
					}
					__int64 nFileTotalSize = Get_FileTotaleSize();
					if ( nFileTotalSize > 0 )
					{
						// �ļ���С��ȥδ��ɵģ����������ص�
						__int64 nHasDownedSize = nFileTotalSize - nTotalUndownloadBytes;

						//////////////////////////////////////////////////////////////////////////
						NotifyData *pCallBackInfo = new NotifyData;
						pCallBackInfo->nID = GetSerialNum();
						pCallBackInfo->cbCurPos = nHasDownedSize; /*Get_TotalDownloadedSize()*/ /* ��ʱ����������δˢ�£�����ֱ��ʹ�ô˺��� */
						pCallBackInfo->cbTotle = nFileTotalSize;
						DWORD dwDownloadElapsedTime = GetDownloadElapsedTime();
						if(dwDownloadElapsedTime <= 0)
						{
							pCallBackInfo->cbSpeedPerSecond = 0;
						}
						else
						{
							pCallBackInfo->cbSpeedPerSecond = int((nTotalDownloadedSize_ThisTimes / dwDownloadElapsedTime) * 1000);
						}

						//���ÿ���ϱ�һ��
						if(GetTickCount() > m_last_report_time + 1000)
						{
							Notify ( -1, GetSerialNum(), NOTIFY_TYPE_HAS_GOT_NEW_DOWNLOADED_DATA, (LPVOID)pCallBackInfo );
							m_last_report_time = GetTickCount();
						}
					}
				}
			}
		}
	}
}

std::wstring CDownloadManager::GetHost(const CString& url)
{
	CString ret;
	int npos = url.Find(_T("://"));
	if(npos == -1)
	{
		return _T("");
	}
	
	ret = url.Right(url.GetLength() - npos - 3);
	int rPos = ret.Find(_T("/"));

	if(rPos != -1)
	{
		ret = ret.Left(rPos);
	}
	return ret.GetBuffer(ret.GetLength());
}

CString CDownloadManager::ReplaceHost(const CString& url, const CString& host)
{
	CString ret;
	int nPos = url.Find(_T("://"));
	if(nPos == -1)
	{
		ret = url;
		return ret;
	}
	ret += url.Left(nPos + 3);
	ret += host;
	int rPos = url.Find(_T("/"), nPos + 3);
	if(rPos != -1)
	{
		ret += url.Right(url.GetLength()- rPos);
	}

	return ret;
}

CString CDownloadManager::GetRandUrl()
{
	//ʹ��Ȩ�أ������
	if(m_pid_info.servers_.size() > 0)
	{
		//�����Ǳ�host������servers�б�����滻host
		BOOL found = FALSE;
		std::wstring host = GetHost(m_csDownloadURL);
		for(std::vector<ServerHost>::iterator itr = m_pid_info.servers_.begin(); itr != m_pid_info.servers_.end(); ++itr)
		{
			if(host == itr->server_)
			{
				found = TRUE;
				break;
			}
		}
		if(!found)
		{
			return m_csDownloadURL;
		}


		int total_weight = 0;
		for(std::vector<ServerHost>::iterator itr = m_pid_info.servers_.begin(); itr != m_pid_info.servers_.end(); ++itr)
		{
			total_weight += itr->weight_;
		}
		if(total_weight == 0)
		{
			total_weight = 1;
		}

		int random_count = rand() % total_weight;
		int seq = 0;
		int pre_weight = 0;
		for(std::vector<ServerHost>::iterator itr = m_pid_info.servers_.begin(); itr != m_pid_info.servers_.end(); ++itr)
		{
			if(random_count < itr->weight_ + pre_weight)
			{
				break;
			}
			pre_weight += itr->weight_;
			seq++;
		}


		CString szLog;
		szLog.Format(_T("GetRandUrl, seq: %d, host: %s"), seq, m_pid_info.servers_[seq].server_.c_str());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return ReplaceHost(m_csDownloadURL, m_pid_info.servers_[seq].server_.c_str());
	}
	else
	{
		return m_csDownloadURL;
	}

}
//
// �������߳�����ʹ�õĶ�������ݻ���
//
BOOL CDownloadManager::CreateDownloadObjectAndDataMTR ()
{
	DeleteDownloadObjectAndDataMTR ();

	_ASSERT ( !m_pDownloadPub_MTR && m_pDownloadPub_Info );
	m_pDownloadPub_MTR = CreateDownloadObject ( m_nThreadCount );

	if (m_pDownloadPub_MTR)
	{
		m_pDownloadPub_MTR->SetTaskID(m_nSerialNum);
	}
	// ���ö��߳�����ʹ�õĶ���Ĳ���
	if ( m_pDownloadPub_MTR )
	{
		for ( int nIndex=0; nIndex<m_nThreadCount; nIndex++ )
		{
			m_pDownloadPub_MTR[nIndex].SetTaskID(m_nSerialNum);
			m_pDownloadPub_MTR[nIndex].m_nIndex = nIndex;
			m_pDownloadPub_MTR[nIndex].m_pDownloadMTR = this;
			m_pDownloadPub_MTR[nIndex].Set_SaveDownloadInfo_Callback ( ::Callback_SaveDownloadInfo, WPARAM(this) );
			m_pDownloadPub_MTR[nIndex].SetAuthorization ( m_pDownloadPub_Info->Get_UserName(), m_pDownloadPub_Info->Get_GetPassword() );
			m_pDownloadPub_MTR[nIndex].SetDownloadUrl (GetRandUrl());
			m_pDownloadPub_MTR[nIndex].SetObserver(m_pObserver);
			if ( !m_pDownloadPub_MTR[nIndex].SetSaveFileName ( GetTempFilePath() ) )
				return FALSE;
		}
	}

	// �������߳�����ʹ�õ����ݻ���
	_ASSERT ( !m_pDownloadCellInfo );
	m_pDownloadCellInfo = new t_DownloadCellInfo[m_nThreadCount];
	if ( m_pDownloadCellInfo )
		memset ( m_pDownloadCellInfo, 0, m_nThreadCount*sizeof(t_DownloadCellInfo) );

	if ( m_pDownloadPub_MTR != NULL && m_pDownloadCellInfo != NULL )
		return TRUE;

	CString szLog = _T("Create download object or buffer failed.");
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
	return FALSE;

}

//
// ɾ�����߳�����ʹ�õĶ�������ݻ���
//
void CDownloadManager::DeleteDownloadObjectAndDataMTR()
{
	if ( m_pDownloadPub_MTR )
	{
		DeleteDownloadObject ( m_pDownloadPub_MTR );
		m_pDownloadPub_MTR = NULL;
	}
	if ( m_pDownloadCellInfo )
	{
		delete[] m_pDownloadCellInfo;
		m_pDownloadCellInfo = NULL;
	}
}

//
// ɾ��ȡվ����Ϣ�����ض���
//
void CDownloadManager::DeleteDownloadObject_Info()
{
	if ( m_pDownloadPub_Info )
	{
		DeleteDownloadObject ( m_pDownloadPub_Info );
		m_pDownloadPub_Info = NULL;
	}
}

//
// �������߳����أ�����m_pDownloadPub_Info��ȡվ����Ϣ��Ȼ�������������߳�
//
NsResultEnum CDownloadManager::StartMTRDownload ()
{
	m_dwDownloadStartTime = GetTickCount();

	////////////////////////////////////////////////////////////////////////////
	////���Ϳ�ʼ���ص���Ϣ
	//if(IsCallBackEnable())
	//{
	//	t_DownloadNotifyPara DownloadNotifyPara = {0};
	//	DownloadNotifyPara.nIndex = -1;
	//	DownloadNotifyPara.nNotityType = NOTIFY_TYPE_HAS_START;
	//	DownloadNotifyPara.lpNotifyData = NULL;
	//	DownloadNotifyPara.nSerialNum = GetSerialNum();
	//	if(m_pObserver != NULL)
	//	{
	//		m_pObserver->OnNotify(&DownloadNotifyPara, NULL);
	//	}
	//}


	//////////////////////////////////////////////////////////////////////////
	// �Ȼ�ȡվ����Ϣ���ļ��������ȣ�md5ֵ
	_ASSERT ( m_pDownloadPub_Info );
	if ( !m_pDownloadPub_Info->GetRemoteSiteInfo () )
	{
		CString szLog = _T("Get remote web site infomation failed.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return RS_ERR_GET_WEBINFO_FAILED;
	}


	CString szLog;
	szLog.Format(_T("The file size to be downloaded is: %I64d bytes."), m_pDownloadPub_Info->Get_FileTotalSize());
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	StandardSaveFileName ();

	if ( ::WaitForSingleObject ( m_hEvtEndModule, 100 ) == WAIT_OBJECT_0 )
	{
		szLog = _T("User canceled after get the remotesite info.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return RS_ERR_USER_CANCEL;
	}

	//�����ѻ���ļ�����С����Ϣ
	BOOL bMustCreateNullFile = TRUE;
	if ( m_bForceDownload )
	{
		// ��Ҫ��������
		::DeleteFile ( m_csSavePathFileName );
		::DeleteFile ( GetTempFilePath() );
	}
	else
	{
		BOOL bNeedReDownLoad = TRUE;
		// ����Ŀ���ļ��Ƿ���ڣ����Ѿ����ڣ������Ƿ���ҪУ��MD5���ֱ���
		HANDLE hFile = ::CreateFile(m_csSavePathFileName,	// file to open
									GENERIC_READ,			// open for read
									FILE_SHARE_READ,		// share for reading
									NULL,					// default security
									OPEN_EXISTING,			// existing file only
									FILE_ATTRIBUTE_NORMAL,	// normal file
									NULL);					// no attr. template
		if (hFile != INVALID_HANDLE_VALUE) 
		{ 
			// �ļ�����
			if(IsNeedCheckMD5() && (GetMD5_WebSite() != _T("")))
			{
				// ��Ҫ��֤MD5
				// ��ȡMD5
				CString szDownLoadFileMD5 = GetFileMD5(m_csSavePathFileName.GetBuffer());
				if(!szDownLoadFileMD5.IsEmpty())
				{
					if(0 == szDownLoadFileMD5.Compare(GetMD5_WebSite()))
					{
						bNeedReDownLoad = FALSE;
					}			
				}
			}
			else
			{
				// ����Ҫ��֤MD5������֤�ļ���С����
				// ��ȡ�ļ���С
				LARGE_INTEGER dwFileSize;
				if(::GetFileSizeEx(hFile, &dwFileSize))
				{
					if(dwFileSize.QuadPart == m_pDownloadPub_Info->Get_FileTotalSize())
					{
						bNeedReDownLoad = FALSE;
					}
				}
			}
			CLOSE_HANDLE(hFile);
		}

		if(!bNeedReDownLoad)
		{
			szLog = _T("Not need a new download.");
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			return RS_NO_NEED_DOWNLOAD;
		}

		//////////////////////////////////////////////////////////////////////////
		//��Ҫ���أ��ȶ�ȡ������Ϣ
		szLog = _T("Need download.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

		// ��ȡ������Ϣ������ܳɹ�������˵���ϴ�������δ���
		if ( m_pDownloadPub_Info->Is_SupportResume() )
		{
			// ������ʱ�ļ��Ƿ���ڣ����Ѵ��ڣ���ȡ������Ϣ
			HANDLE hFile = ::CreateFile(GetTempFilePath(),						// file to open
				GENERIC_READ,							// open for read
				FILE_SHARE_READ,						// share for reading
				NULL,									// default security
				OPEN_EXISTING,							// existing file only
				FILE_ATTRIBUTE_NORMAL,					// normal file
				NULL);									// no attr. template
			if (hFile != INVALID_HANDLE_VALUE) 
			{ 
				// ��ȡ��ʱ�ļ���С
				LARGE_INTEGER dwFileSize;
				if(::GetFileSizeEx(hFile, &dwFileSize))
				{
					if(dwFileSize.QuadPart == m_pDownloadPub_Info->Get_FileTotalSize() + GetDownloadInfoWholeSize())
					{
						if ( ReadDownloadInfo () )
						{
							szLog = _T("ReadDownloadInfo succ.");
							__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
							bMustCreateNullFile = FALSE;
						}
						else
						{
							szLog = _T("ReadDownloadInfo fail.");
							__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
						}
					}
				}
				else
				{
					szLog = _T("GetFileSizeEx failed.");
					__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
				}
				CLOSE_HANDLE(hFile);
			}
			//else
			//{
			//	szLog.Format(_T("CreateFile return INVALID_HANDLE_VALUE, error: %d"), GetLastError());
			//	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			//}
		}
		else
		{
			szLog = _T("Not Support Resume.");
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		}
	}

	if ( bMustCreateNullFile )
	{
		szLog = _T("Must create a null file.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

		__int64 nFileSize = m_pDownloadPub_Info->Get_FileTotalSize();
		__int64 nTempFileSize = nFileSize+GetDownloadInfoWholeSize();
		if ( nFileSize < 0 || !m_pDownloadPub_Info->Is_SupportResume() )
		{
			szLog.Format(_T("set filesize to zero, org: %ld"), nFileSize);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			nTempFileSize = 0;
		}
		// ����һ�����������������ݵĿ��ļ�
		unsigned int nRet = CreateNullFile ( GetTempFilePath(), nTempFileSize );
		if(nRet != 0)
		{
			szLog.Format(_T("CreateNullFile [%s] failed, nRet: %d, size: %I64d"), GetTempFilePath(), nRet, nTempFileSize);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			// create failed
			if(nRet == 2)
			{
				return RS_ERR_DISK_SPACE_NOT_ENOUGH;
			}
			else
			{
				return RS_ERR_FILE_OPRATION_FAILD;
			}
		}
	}
	else
	{
		szLog = _T("Not need create a null file.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	}

	// ������ʱ�ļ�����Ϊ����
	::SetFileAttributes ( GetTempFilePath(), FILE_ATTRIBUTE_HIDDEN );

	//////////////////////////////////////////////////////////////////////////
	// ������������
	if ( !AssignDownloadTask () )
	{
		szLog = _T("Assign task failed.\r\n<<<--------------------");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
		return RS_ERR_TASK_ALLOC_FAILED;
	}
	else
	{
		szLog = _T("Assign task successful.\r\n<<<--------------------");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	}

	m_dwDownloadStartTime = GetTickCount();
	return RS_SUCCESS;
}

//
// �õ���ʱ���ݱ����·���ļ���
//
CString CDownloadManager::GetTempFilePath ()
{
	_ASSERT ( !m_csSavePathFileName.IsEmpty () );
	CString csTempFileName;
	csTempFileName.Format ( _T("%s.~P2S"), m_csSavePathFileName );
	return csTempFileName;
}

//
// ������������
//
BOOL CDownloadManager::AssignDownloadTask()
{
	_ASSERT ( m_pDownloadPub_Info );

	CString szLog;
	//�����֧�ֶϵ���������������¿�ʼ����
	if ( !m_pDownloadPub_Info->Is_SupportResume() )
	{
		DeleteDownloadObjectAndDataMTR ();

		szLog.Format(_T("The website is not support resume download [%s]."), m_pDownloadPub_Info->Get_ServerName());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
	}
	// �ļ���Сδ֪�����õ��߳�
	if ( m_pDownloadPub_Info->Get_FileTotalSize () <= 0 || !m_pDownloadPub_Info->Is_SupportResume() )
	{
		if ( m_nThreadCount != 1 )
		{
			DeleteDownloadObjectAndDataMTR ();
			SetThreadCount ( 1 );
		}

		szLog = _T("The website is not support resume or can not get file size.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
	}

	//���û������������Ϣ������Ҫ����new���ض���
	if ( !DownloadInfoIsValid() || !m_pDownloadPub_MTR || !m_pDownloadCellInfo )
	{
		if ( !CreateDownloadObjectAndDataMTR () )
			return FALSE;
	}

	_ASSERT ( m_pDownloadPub_MTR && m_pDownloadCellInfo );

	// ����������δ����
	szLog = _T("Assign task:\r\n-------------------->>>");
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	//��Ч����Ҫ���¸�ֵ
	if ( !DownloadInfoIsValid() )
	{
		//////////////////////////////////////////////////////////////////////////
		//����з��������ݣ���ʹ�÷��������ص����ݽ��з���
		if(m_nThreadCount > 1 && m_pDownloadPub_Info->Is_SupportResume() && m_pid_info.part_infos_.size()>0 )
		{
			_ASSERT(m_pid_info.part_infos_.size() == m_nThreadCount);
			for ( int nIndex=0; nIndex<m_nThreadCount; nIndex++ )
			{
				m_pDownloadCellInfo[nIndex].nWillDownloadStartPos = m_pid_info.part_infos_[nIndex].offset_;
				m_pDownloadCellInfo[nIndex].nWillDownloadSize = m_pid_info.part_infos_[nIndex].length_;
			}
		}
		else
		{
			//ʹ��Ĭ�ϵķ��䷽ʽ
			__int64 nWillDownloadSize = -1, nWillDownloadStartPos = 0, nNoAssignSize = 0;
			if ( m_pDownloadPub_Info->Get_FileTotalSize () > 0 )
			{
				nWillDownloadSize = m_pDownloadPub_Info->Get_FileTotalSize () / m_nThreadCount;
				// ���ֺ�ʣ�µĲ��֣��õ�һ���߳����е�����
				nNoAssignSize = m_pDownloadPub_Info->Get_FileTotalSize () % m_nThreadCount;
			}

			for ( int nIndex=0; nIndex<m_nThreadCount; nIndex++ )
			{
				m_pDownloadCellInfo[nIndex].nWillDownloadStartPos = nWillDownloadStartPos;
				m_pDownloadCellInfo[nIndex].nWillDownloadSize = nWillDownloadSize;
				if ( nIndex == 0 && m_pDownloadPub_Info->Get_FileTotalSize () > 0 )
				{
					m_pDownloadCellInfo[nIndex].nWillDownloadSize += nNoAssignSize;
				}

				szLog = _T("");
				szLog.Format(_T("Download thread(%d) will download from %I64d to %I64d, Totol: %I64d bytes."), nIndex, 
					m_pDownloadCellInfo[nIndex].nWillDownloadStartPos, 
					//m_pDownloadCellInfo[nIndex].nWillDownloadStartPos,
					m_pDownloadCellInfo[nIndex].nWillDownloadStartPos+m_pDownloadCellInfo[nIndex].nWillDownloadSize,
					//m_pDownloadCellInfo[nIndex].nWillDownloadStartPos+m_pDownloadCellInfo[nIndex].nWillDownloadSize,
					//m_pDownloadCellInfo[nIndex].nWillDownloadSize,
					m_pDownloadCellInfo[nIndex].nWillDownloadSize);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

				nWillDownloadStartPos += m_pDownloadCellInfo[nIndex].nWillDownloadSize;
			}
		}

		
	}


	// ������������
	szLog.Format(_T("Start download sub thread, total: %d"), m_nThreadCount);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	for ( int nIndex=0; nIndex<m_nThreadCount; nIndex++ )
	{
		if ( !m_pDownloadPub_MTR[nIndex].Download ( m_pDownloadCellInfo[nIndex].nWillDownloadStartPos,
			m_pDownloadCellInfo[nIndex].nWillDownloadSize, m_pDownloadCellInfo[nIndex].nDownloadedSize) )
			return FALSE;
	}

	m_BaseDownInfo.dwThreadCount = m_nThreadCount;
	return TRUE;
}

/*
//
// ��������Ϣ�ļ��ж�ȡ������Ϣ
//
BOOL CDownloadManager::ReadDownloadInfo()
{
	CString csTempFileName = GetTempFilePath ();
	BOOL bRet = FALSE;

	// �����ļ��Ƿ����
	HANDLE hFile = ::CreateFile(csTempFileName,								// file to open
		GENERIC_READ,								// open for read
		FILE_SHARE_READ,							// share for reading
		NULL,										// default security
		OPEN_EXISTING,								// existing file only
		FILE_ATTRIBUTE_NORMAL,						// normal file
		NULL);										// no attr. template
	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER dwFileSize;
		DWORD dwHigh = 0;
		dwFileSize.LowPart = ::GetFileSize(hFile, &dwHigh);
		if(dwFileSize.LowPart != INVALID_FILE_SIZE)
		{
			dwFileSize.HighPart = dwHigh;

			LARGE_INTEGER it;
			it.QuadPart = -(int)sizeof(t_BaseDownInfo);
			it.LowPart = ::SetFilePointer (hFile, it.QuadPart, &it.HighPart, FILE_END);
			if(INVALID_SET_FILE_POINTER != it.LowPart)
			{
				if(it.QuadPart == dwFileSize.QuadPart - sizeof(t_BaseDownInfo))
				{
					DWORD dwNumberOfBytesRead = 0;
					if(::ReadFile(hFile, &m_BaseDownInfo, sizeof(t_BaseDownInfo), &dwNumberOfBytesRead, NULL))
					{
						// read success
						if(dwNumberOfBytesRead == sizeof(t_BaseDownInfo))
						{
							if ( (m_BaseDownInfo.dwThreadCount > 0 && m_BaseDownInfo.dwThreadCount <= MAX_DOWNLOAD_THREAD_COUNT)&&
								SetThreadCount ( m_BaseDownInfo.dwThreadCount ) )
							{
								if ( CreateDownloadObjectAndDataMTR () )
								{
									LARGE_INTEGER li;
									li.QuadPart = -GetDownloadInfoWholeSize();
									li.LowPart = ::SetFilePointer (hFile, li.QuadPart, &li.HighPart, FILE_END);
									if(INVALID_SET_FILE_POINTER != li.LowPart)
									{
										if(li.QuadPart == (dwFileSize.QuadPart - GetDownloadInfoWholeSize()))
										{
											DWORD dwRead = 0;
											if(::ReadFile(hFile, m_pDownloadCellInfo, sizeof(t_DownloadCellInfo)*m_nThreadCount, &dwRead, NULL))
											{
												// read success
												if(dwRead == sizeof(t_DownloadCellInfo)*m_nThreadCount)
												{
													bRet = TRUE;
												}
												else
												{
													memset ( m_pDownloadCellInfo, 0, sizeof(t_DownloadCellInfo)*m_nThreadCount );
												}
											}
											else
											{
												memset ( m_pDownloadCellInfo, 0, sizeof(t_DownloadCellInfo)*m_nThreadCount );
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	CLOSE_HANDLE(hFile);
	return bRet;
}
*/


//
// ��������Ϣ�ļ��ж�ȡ������Ϣ
//
BOOL CDownloadManager::ReadDownloadInfo()
{
	CString csTempFileName = GetTempFilePath ();
	BOOL bRet = FALSE;

	// �����ļ��Ƿ����
	HANDLE hFile = ::CreateFile(csTempFileName,								// file to open
								GENERIC_READ,								// open for read
								FILE_SHARE_READ,							// share for reading
								NULL,										// default security
								OPEN_EXISTING,								// existing file only
								FILE_ATTRIBUTE_NORMAL,						// normal file
								NULL);										// no attr. template
	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER dwFileSize;
		if(::GetFileSizeEx(hFile, &dwFileSize))
		{
			// success
			LARGE_INTEGER iToMove;
			LARGE_INTEGER iNewPos;
			iToMove.QuadPart = -(int)sizeof(t_BaseDownInfo);
			if(::SetFilePointerEx (hFile, iToMove, &iNewPos, FILE_END))
			{
				_ASSERT(iNewPos.QuadPart == dwFileSize.QuadPart - sizeof(t_BaseDownInfo));
				DWORD dwNumberOfBytesRead = 0;
				if(::ReadFile(hFile, &m_BaseDownInfo, sizeof(t_BaseDownInfo), &dwNumberOfBytesRead, NULL))
				{
					// read success
					if(dwNumberOfBytesRead == sizeof(t_BaseDownInfo))
					{
						if ( (m_BaseDownInfo.dwThreadCount > 0 && m_BaseDownInfo.dwThreadCount <= MAX_DOWNLOAD_THREAD_COUNT)&&
							SetThreadCount ( m_BaseDownInfo.dwThreadCount ) )
						{
							if ( CreateDownloadObjectAndDataMTR () )
							{
								LARGE_INTEGER liToMove;
								LARGE_INTEGER liNewPos;
								liToMove.QuadPart = -GetDownloadInfoWholeSize();
								if(::SetFilePointerEx(hFile, liToMove, &liNewPos, FILE_END))
								{
									_ASSERT(liNewPos.QuadPart == (dwFileSize.QuadPart - GetDownloadInfoWholeSize()));
									DWORD dwRead = 0;
									if(::ReadFile(hFile, m_pDownloadCellInfo, sizeof(t_DownloadCellInfo)*m_nThreadCount, &dwRead, NULL))
									{
										// read success
										if(dwRead == sizeof(t_DownloadCellInfo)*m_nThreadCount)
										{
											CString szLog;
											szLog.Format(_T("load total download thread: %d"), m_BaseDownInfo.dwThreadCount);
											__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
											for(int i = 0; i < m_nThreadCount; ++ i)
											{
												CString szLog;
												szLog.Format(_T("DownCellInfo (%d), start: %I64d, need: %I64d, downed: %I64d"),
														i, m_pDownloadCellInfo[i].nWillDownloadStartPos,
														m_pDownloadCellInfo[i].nWillDownloadSize,
														m_pDownloadCellInfo[i].nDownloadedSize);
												__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
											}
											bRet = TRUE;
										}
										else
										{
											memset ( m_pDownloadCellInfo, 0, sizeof(t_DownloadCellInfo)*m_nThreadCount );
										}
									}
									else
									{
										memset ( m_pDownloadCellInfo, 0, sizeof(t_DownloadCellInfo)*m_nThreadCount );
									}
								}
							}
						}
					}
				}
			}
		}
	}

	CLOSE_HANDLE(hFile);
	return bRet;
}

/*
BOOL CDownloadManager::SaveDownloadInfo ()
{
	if ( !m_pDownloadPub_Info->Is_SupportResume() )
		return TRUE;
	CString csTempFileName = GetTempFilePath ();
	BOOL bRet = FALSE;

	// �����ļ��Ƿ����
	HANDLE hFile = ::CreateFile(csTempFileName,	// file to open
		GENERIC_READ | GENERIC_WRITE,			// open for read write
		FILE_SHARE_READ | FILE_SHARE_WRITE,		// share for reading
		NULL,									// default security
		OPEN_EXISTING,							// existing file only
		FILE_ATTRIBUTE_NORMAL,					// normal file
		NULL);									// no attr. template
	if (hFile == INVALID_HANDLE_VALUE) 
	{ 
		// �ļ�������
		return FALSE;
	}

	{
		LARGE_INTEGER dwFileSize;
		DWORD dwHigh = 0;
		dwFileSize.LowPart = ::GetFileSize(hFile, &dwHigh);
		if((dwFileSize.LowPart == INVALID_FILE_SIZE) || (dwFileSize.LowPart <= 0))
		{
			CLOSE_HANDLE(hFile);
			return FALSE;
		}

		dwFileSize.HighPart = dwHigh;
		LARGE_INTEGER it;
		it.QuadPart = -(int)sizeof(t_BaseDownInfo);
		it.LowPart = ::SetFilePointer (hFile, it.QuadPart, &it.HighPart, FILE_END);
		if(INVALID_SET_FILE_POINTER != it.LowPart)
		{
			if(it.QuadPart == (dwFileSize.QuadPart - sizeof(t_BaseDownInfo)))
			{
				DWORD dwNumberOfBytesWritten = 0;
				if(::WriteFile(hFile, &m_BaseDownInfo, sizeof(t_BaseDownInfo),&dwNumberOfBytesWritten, NULL))
				{
					//success
				}

				//
				LARGE_INTEGER li;
				li.QuadPart = -GetDownloadInfoWholeSize();
				li.LowPart = ::SetFilePointer (hFile, li.QuadPart, &li.HighPart, FILE_END);
				if(INVALID_SET_FILE_POINTER != li.LowPart)
				{
					if(li.QuadPart == (dwFileSize.QuadPart - GetDownloadInfoWholeSize()))
					{
						DWORD dwWritten = 0;
						if(::WriteFile(hFile, m_pDownloadCellInfo, m_nThreadCount*sizeof(t_DownloadCellInfo), &dwWritten, NULL))
						{
							//success
						}
						bRet = TRUE;
					}
				}
			}
		}
	}
	CLOSE_HANDLE(hFile);

	if ( !bRet )
	{
		CString szLog;
		szLog.Format(_T("Save download infomation failed. GetLastError : %s."), hwFormatMessage ( GetLastError() ));
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
	}
	return bRet;
}
*/

BOOL CDownloadManager::SaveDownloadInfo ()
{
	if ( !m_pDownloadPub_Info->Is_SupportResume() )
	{
		return TRUE;
	}

	CString csTempFileName = GetTempFilePath ();
	BOOL bRet = FALSE;

	// �����ļ��Ƿ����
	HANDLE hFile = ::CreateFile(csTempFileName,							// file to open
								GENERIC_READ | GENERIC_WRITE,			// open for read write
								FILE_SHARE_READ | FILE_SHARE_WRITE,		// share for reading
								NULL,									// default security
								OPEN_EXISTING,							// existing file only
								FILE_ATTRIBUTE_NORMAL,					// normal file
								NULL);									// no attr. template
	if (hFile == INVALID_HANDLE_VALUE) 
	{ 
		// �ļ�������
		return FALSE;
	}

	if(m_BaseDownInfo.dwThreadCount == 0 || m_pDownloadCellInfo == NULL)
	{
		CString szLog;
		szLog.Format(_T("m_pDownloadCellInfo == NULL or m_BaseDownInfo.dwThreadCount= %d."), m_BaseDownInfo.dwThreadCount);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
		CLOSE_HANDLE(hFile);
		return FALSE;
	}

	CString szLog;
	szLog.Format(_T("start save download info, total download thread: %d"), m_BaseDownInfo.dwThreadCount);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	for(int i = 0; i < m_nThreadCount; ++i)
	{
		CString szLog;
		szLog.Format(_T("start save cell info (%d), start: %I64d, need: %I64d, downed: %I64d"), i,
			m_pDownloadCellInfo[i].nWillDownloadStartPos,
			m_pDownloadCellInfo[i].nWillDownloadSize,
			m_pDownloadCellInfo[i].nDownloadedSize);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	}

	LARGE_INTEGER dwFileSize;
	if(::GetFileSizeEx(hFile, &dwFileSize))
	{
		LARGE_INTEGER itToMove;
		LARGE_INTEGER itNewPos;
		itToMove.QuadPart = -(int)sizeof(t_BaseDownInfo);
		if(::SetFilePointerEx(hFile, itToMove, &itNewPos, FILE_END))
		{
			_ASSERT(itNewPos.QuadPart == (dwFileSize.QuadPart - sizeof(t_BaseDownInfo)));

			DWORD dwNumberOfBytesWritten = 0;
			if(::WriteFile(hFile, &m_BaseDownInfo, sizeof(t_BaseDownInfo),&dwNumberOfBytesWritten, NULL))
			{
				//success
			}

			//
			LARGE_INTEGER liToMove;
			LARGE_INTEGER liNewPos;
			liToMove.QuadPart = -GetDownloadInfoWholeSize();
			if(::SetFilePointerEx(hFile, liToMove, &liNewPos, FILE_END))
			{
				_ASSERT(liNewPos.QuadPart == (dwFileSize.QuadPart - GetDownloadInfoWholeSize()));

				DWORD dwWritten = 0;
				if(::WriteFile(hFile, m_pDownloadCellInfo, m_nThreadCount*sizeof(t_DownloadCellInfo), &dwWritten, NULL))
				{
					//success
				}
				bRet = TRUE;
			}
		}
	}


	CLOSE_HANDLE(hFile);
	if ( !bRet )
	{
		CString szLog;
		szLog.Format(_T("Save download infomation failed. GetLastError : %s."), hwFormatMessage ( GetLastError() ));
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
	}
	return bRet;
}


BOOL CDownloadManager::HandleDownloadFinished(NsResultEnum& eDownloadResult)
{
	CString szLog;
	BOOL bRet = FALSE;
	if ( eDownloadResult != RS_SUCCESS )
	{
		szLog.Format(_T("start SaveDownloadInfo."));
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
		SaveDownloadInfo ();
	}
	else
	{
		CString csTempFileName = GetTempFilePath ();

		// ���������ļ�����
		{
			bRet = FALSE;
			// �����ļ��Ƿ����
			HANDLE hFile = ::CreateFile(csTempFileName,							// file to open
										GENERIC_READ | GENERIC_WRITE,			// open for read write
										FILE_SHARE_READ | FILE_SHARE_WRITE,		// share for reading
										NULL,									// default security
										OPEN_EXISTING,							// existing file only
										FILE_ATTRIBUTE_NORMAL,					// normal file
										NULL);									// no attr. template
			if (hFile != INVALID_HANDLE_VALUE) 
			{ 
				// �ļ�����
				LARGE_INTEGER it;
				it.QuadPart = m_pDownloadPub_Info->Get_FileTotalSize();
				if(::SetFilePointerEx (hFile, it, NULL, FILE_BEGIN))
				{
					if(::SetEndOfFile(hFile))
					{
						// success
						bRet = TRUE;
					}
				}
				CLOSE_HANDLE(hFile);			
			}
			if ( bRet )
			{
				szLog.Format(_T("Set [%s] length successful."), csTempFileName);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			}
			else
			{
				szLog.Format(_T("Set [%s] length failed."), csTempFileName);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
				eDownloadResult = RS_ERR_FILE_OPRATION_FAILD;
			}
		}

		// success go on
		if ( bRet )
		{
			bRet = FALSE;
			// ����ʱ�ļ�������ΪĿ���ļ���
			DeleteFile ( m_csSavePathFileName );
			if(0 != _wrename(csTempFileName, m_csSavePathFileName))
			{
				// rename failed
				szLog.Format(_T("Rename [%s] failed. GetLastError : %s."), csTempFileName, hwFormatMessage(GetLastError()));
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);

				eDownloadResult = RS_ERR_FILE_OPRATION_FAILD;
			}
			else
			{
				bRet = TRUE;
				szLog.Format(_T("Rename [%s] sucessful."), csTempFileName);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			}
		}
		
		// success go on
		if ( bRet )
		{
			// �����ļ�����
			::SetFileAttributes(m_csSavePathFileName, FILE_ATTRIBUTE_NORMAL);

			// ��֤MD5
			if(IsNeedCheckMD5() && GetMD5_WebSite() != _T(""))
			{
				bRet = FALSE;
				BOOL bPartRet = TRUE;
				//////////////////////////////////////////////////////////////////////////
				//��֤�ֶ�md5,ֻ�ж���һ���ֶβż��
				//if(m_pid_info.part_infos_.size() > 1 && m_nThreadCount > 1 && m_pDownloadPub_Info->Is_SupportResume())
				//{
				//	if(!CheckPartMD5())
				//	{
				//		bPartRet = FALSE;
				//	}
				//}

				if(bPartRet)
				{
					//////////////////////////////////////////////////////////////////////////
					// ��ȡMD5
					CString szDownLoadFileMD5 = GetFileMD5(m_csSavePathFileName.GetBuffer());
					if(!szDownLoadFileMD5.IsEmpty())
					{
						szLog.Format(_T("Need Check MD5, FILE[%s]-MD5[%s]"), m_csSavePathFileName, szDownLoadFileMD5);
						__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);


						if(0 == szDownLoadFileMD5.Compare(GetMD5_WebSite()))
						{
							bRet = TRUE;
						}	
						else
						{
							szLog.Format(_T("Check MD5 failed, need: [%s]- but: [%s]"), GetMD5_WebSite(), szDownLoadFileMD5);
							__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
						}
					}
				}
				
				if ( !bRet )
				{
					eDownloadResult = RS_ERR_MD5_CHECK_FAILED;
					::DeleteFile(m_csSavePathFileName);
				}
			}
		}
	}

	szLog.Format(_T("HandleDownloadFinished return: %s, errno: %d"), bRet ? _T("TRUE") : _T("FALSE"), eDownloadResult);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	if(eDownloadResult == RS_SUCCESS)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CDownloadManager::CheckPartMD5()
{
	for(std::vector<PartInfo>::iterator itr = m_pid_info.part_infos_.begin(); itr != m_pid_info.part_infos_.end(); ++itr)
	{
		CString part_md5 = GetPartFileMD5(m_csSavePathFileName.GetBuffer(), itr->offset_, itr->length_);
		CString ReadMD5 = itr->md5_.c_str();
		ReadMD5.MakeUpper();
		if(ReadMD5.Compare(part_md5) != 0)
		{
			CString szLog;
			szLog.Format(_T("Need Check MD5, FILE[%s]-MD5[%s], BUT[%s]"), m_csSavePathFileName, ReadMD5, part_md5);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CDownloadManager::GetDownloadResult()
{
	for ( int nIndex=0; nIndex<m_nThreadCount; nIndex++ )
	{
		if ( !m_pDownloadPub_MTR[nIndex].Is_DownloadSuccess() )
			return FALSE;
	}

	return TRUE;
}

//
// ������Ϣ�Ƿ���Ч
//
BOOL CDownloadManager::DownloadInfoIsValid()
{
	BOOL bValid = FALSE;
	if ( m_pDownloadCellInfo )
	{
		if ( m_BaseDownInfo.dwThreadCount >= 1 && m_BaseDownInfo.dwThreadCount <= MAX_DOWNLOAD_THREAD_COUNT )
		{
			for (int nIndex = 0; nIndex < m_nThreadCount; nIndex++ )
			{
				if ( m_pDownloadCellInfo[nIndex].nWillDownloadSize > 0 )
				{
					bValid = TRUE;
					break;
				}
			}
		}
	}
	
	if ( !bValid )
	{
		if ( m_pDownloadCellInfo )
		{
			memset ( m_pDownloadCellInfo, 0, m_nThreadCount*sizeof(t_DownloadCellInfo) );
		}
		memset ( &m_BaseDownInfo, 0, sizeof(t_BaseDownInfo) );
	}
	return bValid;
}

//
// �ҵ�ʣ��δ���ص����������Ǹ�������, ����ѡ��δ���سɹ����Ǹ�����
//
int CDownloadManager::GetUndownloadMaxBytes( __int64 &nUndownloadBytes )
{
	nUndownloadBytes = 0;
	int nMaxIndex = -1;
	for ( int nIndex = 0; nIndex < m_nThreadCount; nIndex++ )
	{
		// ����ѡ��δ���سɹ����Ǹ�����
		if(!m_pDownloadPub_MTR[nIndex].ThreadIsRunning())
		{
			if(!m_pDownloadPub_MTR[nIndex].Is_DownloadSuccess())
			{
				nUndownloadBytes = m_pDownloadPub_MTR[nIndex].GetUndownloadBytes ();
				nMaxIndex = nIndex;
				break;
			}
		}

		__int64 nTempBytes = m_pDownloadPub_MTR[nIndex].GetUndownloadBytes ();
		if ( nUndownloadBytes < nTempBytes )
		{
			nUndownloadBytes = nTempBytes;
			nMaxIndex = nIndex;
		}
	}
	return nMaxIndex;
}

//
// ���Ϊ nIndex �Ķ����������Ϊ����������صĶ�����Ḻ��
//
BOOL CDownloadManager::AttemperDownloadTask(int nIndex)
{
	_ASSERT ( m_pDownloadPub_MTR && m_pDownloadCellInfo );
	if ( m_nThreadCount <= 1 || m_pDownloadCellInfo[nIndex].nWillDownloadSize == -1 )
	{
		return FALSE;
	}

	__int64 nUndownloadBytes = 0;
	int nIndex_Heavy = GetUndownloadMaxBytes ( nUndownloadBytes );
	if ( nIndex_Heavy == -1 || nIndex_Heavy == nIndex )
	{
		return FALSE;
	}

	//���ʣ���������ݲ���100K������ҪЭ������
	if ( m_pDownloadPub_MTR[nIndex_Heavy].ThreadIsRunning() && nUndownloadBytes < 100*1024 )
	{
		return FALSE;
	}

	_ASSERT ( nIndex_Heavy >= 0 && nIndex_Heavy < m_nThreadCount );
	_ASSERT ( m_pDownloadPub_MTR[nIndex_Heavy].Get_WillDownloadStartPos() == m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos );

	CString szLog;
	szLog.Format(_T("Download thread(%d) help thread(id:%d status:%s)."), nIndex, nIndex_Heavy, m_pDownloadPub_MTR[nIndex_Heavy].ThreadIsRunning()?_T("running"):_T("stopped"));
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	// ���������ض������������
	m_pDownloadCellInfo[nIndex].nWillDownloadSize = ( m_pDownloadPub_MTR[nIndex_Heavy].ThreadIsRunning()?(nUndownloadBytes/2) : nUndownloadBytes );
	m_pDownloadCellInfo[nIndex].nWillDownloadStartPos = m_pDownloadPub_MTR[nIndex_Heavy].Get_WillDownloadStartPos() +
		m_pDownloadPub_MTR[nIndex_Heavy].Get_WillDownloadSize() - m_pDownloadCellInfo[nIndex].nWillDownloadSize;
	m_pDownloadCellInfo[nIndex].nDownloadedSize = 0;

	szLog.Format(_T("Free download thread(%d) assign new task: from %I64d(0x%I64x) to %I64d(0x%I64x), Totol %I64d(0x%I64x) bytes."), nIndex, m_pDownloadCellInfo[nIndex].nWillDownloadStartPos, m_pDownloadCellInfo[nIndex].nWillDownloadStartPos,
		m_pDownloadCellInfo[nIndex].nWillDownloadStartPos + m_pDownloadCellInfo[nIndex].nWillDownloadSize, m_pDownloadCellInfo[nIndex].nWillDownloadStartPos + m_pDownloadCellInfo[nIndex].nWillDownloadSize,
		m_pDownloadCellInfo[nIndex].nWillDownloadSize, m_pDownloadCellInfo[nIndex].nWillDownloadSize);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);


	// �����������ض������������
	m_pDownloadPub_MTR[nIndex].ResetVar ();
	if ( !m_pDownloadPub_MTR[nIndex].Download ( m_pDownloadCellInfo[nIndex].nWillDownloadStartPos,
		m_pDownloadCellInfo[nIndex].nWillDownloadSize, m_pDownloadCellInfo[nIndex].nDownloadedSize ) )
	{
		return FALSE;
	}

	// ���ᷱæ���ض��������
	m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize -= m_pDownloadCellInfo[nIndex].nWillDownloadSize;
	m_pDownloadPub_MTR[nIndex_Heavy].Set_WillDownloadSize ( m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize );

	szLog.Format(_T("Download thread(%d) has downloaded %I64d(0x%I64x), Remaining %I64d(0x%I64x) bytes; Adjust task as: from %I64d(0x%I64x) to %I64d(0x%I64x), Totol %I64d(0x%I64x) bytes."), nIndex_Heavy, m_pDownloadPub_MTR[nIndex_Heavy].Get_DownloadedSize(), m_pDownloadPub_MTR[nIndex_Heavy].Get_DownloadedSize(),
		nUndownloadBytes, nUndownloadBytes,
		m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos, m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos,
		m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos + m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize, m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos + m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize,
		m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize, m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	// ��nIndex_Heavy�̶߳�������񱻵��ȸ���nIndex����nIndex_Heavy�����Ѿ��˳��ˣ�����Ҫ�������ر�־����Ϊ�ɹ�
	if(!m_pDownloadPub_MTR[nIndex_Heavy].ThreadIsRunning())
	{
		m_pDownloadPub_MTR[nIndex_Heavy].Set_DownloadSuccess(TRUE);
	}
	return TRUE;
}


int CDownloadManager::FindIndexByThreadHandle(HANDLE hThread)
{
	for ( int nIndex=0; nIndex<m_nThreadCount; nIndex++ )
	{
		HANDLE hThread_Temp = m_pDownloadPub_MTR[nIndex].Get_Thread_Handle ();
		if ( HANDLE_IS_VALID(hThread_Temp) && hThread_Temp == hThread )
			return nIndex;
	}
	return -1;
}

int CDownloadManager::GetDownloadInfoWholeSize()
{
	return ( sizeof(t_DownloadCellInfo)*m_nThreadCount + sizeof(t_BaseDownInfo) );
}

//
// ��ȡ���������ĵ�ʱ�䣨���룩�����������������ٶȺ�����ʣ��ʱ��
//
DWORD CDownloadManager::GetDownloadElapsedTime()
{
	return (GetTickCount() - m_dwDownloadStartTime);
}

//
// ֹͣ���ء������������̹߳رգ������ض���ɾ�����ļ��ر�
//
void CDownloadManager::StopDownload(DWORD dwTimeout)
{
	if ( HANDLE_IS_VALID(m_hEvtEndModule) )
	{
		::SetEvent ( m_hEvtEndModule );
	}
	if ( HANDLE_IS_VALID(m_hThread) )
	{
		WaitForThreadEnd ( m_hThread, dwTimeout );
		CLOSE_HANDLE ( m_hThread )
	}

	DeleteDownloadObjectAndDataMTR ();
	DeleteDownloadObject_Info ();

	CLOSE_HANDLE ( m_hEvtEndModule );
}

void CDownloadManager::StandardSaveFileName ()
{
	_ASSERT ( m_csSavePath.GetLength() > 0 );
	if( m_csSavePath.GetAt(m_csSavePath.GetLength() - 1) != '\\' )
	{
		m_csSavePath += '\\';
	}
	wchar_t szOnlyFileName_NoExt_User[MAX_PATH] = {0};
	wchar_t szExtensionName_User[MAX_PATH] = {0};

	// ����û�ָ�����µı����ļ����������µġ�
	if ( m_csSaveOnlyFileName.GetLength() > 0 )
	{
		CString csFileNameByURL = GetLocalFileNameByURL ( m_csDownloadURL );
		if ( csFileNameByURL.CompareNoCase(m_csSaveOnlyFileName) != 0 )
		{
			PartFileAndExtensionName ( m_csSaveOnlyFileName, szOnlyFileName_NoExt_User, MAX_PATH, szExtensionName_User, MAX_PATH );
		}
	}

	CString csExtensionName_Remote;
	CString csFileName_Remote = m_pDownloadPub_Info->GetDownloadObjectFileName ( &csExtensionName_Remote );
	if ( wcslen(szOnlyFileName_NoExt_User) > 0 )
	{
		//�����׺��Ϊ����Ϊ��
		if (wcslen(szExtensionName_User) < 1)
		{
			m_csSavePathFileName.Format ( _T("%s%s"), StandardizationFileForPathName(m_csSavePath,FALSE),
				StandardizationFileForPathName(szOnlyFileName_NoExt_User,TRUE));
		}
		//������׺��
		else
		{
			m_csSavePathFileName.Format ( _T("%s%s.%s"), StandardizationFileForPathName(m_csSavePath,FALSE),
				StandardizationFileForPathName(szOnlyFileName_NoExt_User,TRUE), StandardizationFileForPathName(szExtensionName_User,TRUE) );
		}
	}
	else
	{
		m_csSavePathFileName.Format ( _T("%s%s"), StandardizationFileForPathName(m_csSavePath,FALSE), StandardizationFileForPathName(csFileName_Remote,TRUE) );
	}
}

//
// ���� URL ����ȡ���ر�����ļ���
//
CString CDownloadManager::GetLocalFileNameByURL ( LPCTSTR lpszDownloadURL )
{
	if ( !lpszDownloadURL || wcslen(lpszDownloadURL) < 1 )
		return _T("");
	WCHAR szOnlyPath[MAX_PATH] = {0};
	WCHAR szOnlyFileName[MAX_PATH] = {0};
	if ( !PartFileAndPathByFullPath ( lpszDownloadURL, szOnlyFileName, MAX_PATH, szOnlyPath, MAX_PATH ) )
		return _T("");
	return szOnlyFileName;
}

//
// ��ȡ�ļ���С
//
__int64 CDownloadManager::Get_FileTotaleSize()
{
	if ( !m_pDownloadPub_Info ) return -1;
	return m_pDownloadPub_Info->Get_FileTotalSize ();
}

//
// ��ȡ�����ص��ֽ�����������ǰ���صĺͱ������ص�
//
__int64 CDownloadManager::Get_TotalDownloadedSize()
{
	//����Ѿ������ֱ�ӷ����ļ���С lr
	if(IsComplete())
	{
		return m_pDownloadPub_Info->Get_FileTotalSize();
	}

	if ( !m_pDownloadPub_Info ) return -1;
	__int64 nTotalUndownloadBytes = 0;

	if(m_pDownloadPub_MTR != NULL)
	{
		for ( int nIndex = 0; nIndex < m_nThreadCount; nIndex++ )
		{
			nTotalUndownloadBytes += m_pDownloadPub_MTR[nIndex].GetUndownloadBytes();
		}
	}
	__int64 nFileSize = m_pDownloadPub_Info->Get_FileTotalSize();
	if ( nFileSize < 1 ) return -1;

	// �ļ���С��ȥδ��ɵģ����������ص�
	return ( nFileSize - nTotalUndownloadBytes );
}

__int64 CDownloadManager::Get_TotalDownloadedSize_ThisTimes()
{
	m_CSFor_DownloadedData.Lock ();
	__int64 nTotalDownloadedSize_ThisTimes = m_nTotalDownloadedSize_ThisTimes;
	m_CSFor_DownloadedData.Unlock ();
	return nTotalDownloadedSize_ThisTimes;
}

int CDownloadManager::SetTaskID(int nPos)
{
	m_nSerialNum = nPos;
	return 1;
}

int CDownloadManager::GetSerialNum()
{
	return m_nSerialNum;
}

//�����µĽ����¼�
//1  ->�ɹ�����
//0  ->ԭ�������ڽ���
//-1 ->����ʧ��
int CDownloadManager::CreatEndEvent()
{
	if(!HANDLE_IS_VALID(m_hEvtEndModule))
	{
		CLOSE_HANDLE ( m_hEvtEndModule );
		m_hEvtEndModule = ::CreateEvent ( NULL, TRUE, FALSE, NULL );
		if(HANDLE_IS_VALID(m_hEvtEndModule))
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		//�����Ч��ԭ�������ڽ���
		return 0;
	}
}

void CDownloadManager::EnableCallBack(BOOL bEnable)
{
	CThreadGuard cs;
	cs.Lock();
	m_EnableCallBack = bEnable;
	cs.Unlock();
}

BOOL CDownloadManager::IsCallBackEnable()
{
	BOOL bEnable = TRUE;
	CThreadGuard cs;
	cs.Lock();
	bEnable = m_EnableCallBack;
	cs.Unlock();
	return bEnable;
}

BOOL CDownloadManager::IsComplete()
{
	BOOL bComplete = TRUE;
	CThreadGuard cs;
	cs.Lock();
	bComplete = m_bComplete;
	cs.Unlock();
	return bComplete;
}

void CDownloadManager::SetComplete(BOOL bComplete)
{
	CThreadGuard cs;
	cs.Lock();
	m_bComplete = bComplete;
	cs.Unlock();
}

//
// �ȴ����ؽ���
//
NsResultEnum CDownloadManager::WaitForDownloadFinished()
{
	_ASSERT ( HANDLE_IS_VALID(m_hEvtEndModule) );

	NsResultEnum eDownloadResult = RS_ERR_UNKOWN;
	HANDLE *lpHandles = new HANDLE[m_nThreadCount + 1];
	if ( lpHandles )
	{
		while ( TRUE )
		{
			int nCount = 0;
			for ( int nIndex = 0; nIndex < m_nThreadCount; nIndex++ )
			{
				HANDLE hThread = m_pDownloadPub_MTR[nIndex].Get_Thread_Handle ();
				if ( HANDLE_IS_VALID(hThread) )
				{
					lpHandles[nCount++] = hThread;
				}
			}
			lpHandles[nCount++] = m_hEvtEndModule;

			//���ֻʣ�����߳�
			if ( nCount == 1 )
			{
				__int64 nSize = Get_TotalDownloadedSize();
				if ( nSize == m_pDownloadPub_Info->Get_FileTotalSize() )
				{
					eDownloadResult = RS_SUCCESS;
				}
				else if(nSize > m_pDownloadPub_Info->Get_FileTotalSize())
				{
					eDownloadResult = RS_ERR_FILE_SIZE_INCORRECT;
				}
				else
				{
					eDownloadResult = RS_ERR_RCV_FAILED;
				}
				break;
			}

			int nRet = (int)WaitForMultipleObjects ( nCount, lpHandles, FALSE, INFINITE ) - WAIT_OBJECT_0;
			if ( nRet >= 0 && nRet < nCount - 1 )
			{
				// ĳ�����߳��˳��ˣ����߳������������Χ
				int nIndex = FindIndexByThreadHandle ( lpHandles[nRet] );
				if ( ( nIndex >= 0 && nIndex < m_nThreadCount ) )
				{
					// �ҵ�������̵߳ı��
					if(m_pDownloadPub_MTR[nIndex].Is_DownloadSuccess())
					{
						// ���߳������سɹ�
						// ���԰����������ص��̣߳���ʧ�ܣ�����߳�����ر�
						if ( !AttemperDownloadTask ( nIndex ) )
						{
							m_pDownloadPub_MTR[nIndex].Clear_Thread_Handle ();
						}
					}
					else
					{
						// ���߳�δ���سɹ�
						// �رմ����񣬵ȴ������̰߳������������
						m_pDownloadPub_MTR[nIndex].Clear_Thread_Handle ();
					}
				}
				else
				{
					eDownloadResult = RS_ERR_UNKOWN;
					break;
				}
			}
			else if(nRet == (nCount - 1))
			{
				// �û�ѡ��ֹͣ���� m_hEvtEndModule���ź�
				CString szLog;
				szLog.Format(_T("User canceled while downloading, %s"), m_csDownloadURL);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

				eDownloadResult = RS_ERR_USER_CANCEL;
				break;
			}
			// ģ�����		
			else
			{
				// ���ε���ʧ�ܣ��ظ��˹���
				eDownloadResult = RS_ERR_UNKOWN;
			}
		}
	}
	

	// ���쳣���ȴ����������߳̽���
	if ( eDownloadResult != RS_SUCCESS )
	{
		if ( m_pDownloadPub_MTR )
		{
			for ( int nIndex = 0; nIndex < m_nThreadCount; nIndex++ )
			{
				CString szLog;
				szLog.Format(_T("stop thread %d, %s"), nIndex, m_csDownloadURL);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
				m_pDownloadPub_MTR[nIndex].StopDownload ();
			}
		}
		if ( m_pDownloadPub_Info )
		{
			CString szLog;
			szLog.Format(_T("stop thread main info, %s"), m_csDownloadURL);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			m_pDownloadPub_Info->StopDownload ();
		}
	}
	if ( lpHandles )
	{
		delete[] lpHandles;
	}
	return eDownloadResult;
}


// �����Ƿ���Ҫ��֤MD5
void CDownloadManager::SetNeedCheckMD5(BOOL bCheck)
{
	m_bNeedCheckMD5 = bCheck;
}

// �Ƿ���Ҫ��֤MD5
BOOL CDownloadManager::IsNeedCheckMD5()
{
	return m_bNeedCheckMD5;
}

// ��վ���ȡ����MD5ֵ
CString CDownloadManager::GetMD5_WebSite()
{
	return m_csMD5_Web;
}

// ����Ҫ�����ļ���MD5ֵ����Ϊ��׼ֵ
void CDownloadManager::SetMD5_WebSite(CString szMD5)
{
	m_csMD5_Web = szMD5;
	m_csMD5_Web.MakeUpper();
	CString szLog;
	szLog.Format(_T("SetMD5_WebSite: [") + m_csMD5_Web + _T("] in function SetMD5_WebSite"));
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);
}

// ��ȡĳ���ļ���MD5ֵ
CString CDownloadManager::GetPartFileMD5(wchar_t *file, __int64 offset, __int64 length)
{
	CString szMD5;
	if(file)
	{
		// �����ļ��Ƿ����
		HANDLE hFile = ::CreateFile(file,			// file to open
			GENERIC_READ,							// open for read
			FILE_SHARE_READ,						// share for reading
			NULL,									// default security
			OPEN_EXISTING,							// existing file only
			FILE_ATTRIBUTE_NORMAL,					// normal file
			NULL);									// no attr. template
		if (hFile != INVALID_HANDLE_VALUE) 
		{ 
			LARGE_INTEGER dwFileSize;
			if(::GetFileSizeEx(hFile, &dwFileSize))
			{
				// ѭ�����ļ�
				int nKit = 2*1024*1024; // 64M ***����Ϊ64��������
				char *buffer = new char[nKit];
				if(buffer)
				{
					memset(buffer, 0, nKit);

					// ��ʼ��MD5�����
					MD5 ob;
					unsigned char digest[16] = {0};
					MD5::MD5_CTX context;
					ob.MD5Init (&context);

					LARGE_INTEGER nSize;
					nSize.QuadPart = length;
					LARGE_INTEGER itToMove;
					itToMove.QuadPart = offset;
					while(nSize.QuadPart > 0)
					{
						if(!::SetFilePointerEx(hFile, itToMove, NULL, FILE_BEGIN))
						{
							break;
						}
						else
						{
							DWORD dwNumberOfBytesRead = 0;
							LONGLONG read_data_size = nKit > nSize.QuadPart ? nSize.QuadPart : nKit;
							if(::ReadFile(hFile, buffer, (DWORD)read_data_size, &dwNumberOfBytesRead, NULL))
							{
								// read success
								if(0 == dwNumberOfBytesRead)
								{
									// end of file
									break;
								}
								else
								{
									ob.MD5Update (&context, (unsigned char*)buffer, dwNumberOfBytesRead);
									itToMove.QuadPart += dwNumberOfBytesRead;
									nSize.QuadPart -= dwNumberOfBytesRead;
								}
							}
							else
							{
								break;
							}
						}
					}

					if(itToMove.QuadPart == offset + length)
					{
						ob.MD5Final (digest, &context);
						string sz = ob.ResultToHex(digest, true);
						szMD5 = sz.c_str();
					}

					delete buffer;
					buffer = NULL;
				}
			}
			CloseHandle(hFile);			
		}
	}

	szMD5.MakeUpper();
	if(file)
	{
		CString szLog;
		szLog.Format(_T("File(%s) - MD5(%s)."), file, szMD5);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	}
	return szMD5;
}

// ��ȡĳ���ļ���MD5ֵ
CString CDownloadManager::GetFileMD5(wchar_t *file)
{
	CString szMD5;
	if(file)
	{
		// �����ļ��Ƿ����
		HANDLE hFile = ::CreateFile(file,			// file to open
			GENERIC_READ,							// open for read
			FILE_SHARE_READ,						// share for reading
			NULL,									// default security
			OPEN_EXISTING,							// existing file only
			FILE_ATTRIBUTE_NORMAL,					// normal file
			NULL);									// no attr. template
		if (hFile != INVALID_HANDLE_VALUE) 
		{ 
			LARGE_INTEGER dwFileSize;
			if(::GetFileSizeEx(hFile, &dwFileSize))
			{
				// ѭ�����ļ�
				int nKit = 64*1024*1024; // 64M ***����Ϊ64��������
				char *buffer = new char[nKit];
				if(buffer)
				{
					memset(buffer, 0, nKit);

					// ��ʼ��MD5�����
					MD5 ob;
					unsigned char digest[16] = {0};
					MD5::MD5_CTX context;
					ob.MD5Init (&context);

					LARGE_INTEGER nSize;
					nSize.QuadPart = dwFileSize.QuadPart;
					LARGE_INTEGER itToMove;
					itToMove.QuadPart = 0;
					while(nSize.QuadPart > 0)
					{
						if(!::SetFilePointerEx(hFile, itToMove, NULL, FILE_BEGIN))
						{
							break;
						}
						else
						{
							DWORD dwNumberOfBytesRead = 0;
							if(::ReadFile(hFile, buffer, nKit, &dwNumberOfBytesRead, NULL))
							{
								// read success
								if(0 == dwNumberOfBytesRead)
								{
									// end of file
									break;
								}
								else
								{
									ob.MD5Update (&context, (unsigned char*)buffer, dwNumberOfBytesRead);
									itToMove.QuadPart += dwNumberOfBytesRead;
									nSize.QuadPart -= dwNumberOfBytesRead;
								}
							}
							else
							{
								break;
							}
						}
					}

					if(itToMove.QuadPart == dwFileSize.QuadPart)
					{
						ob.MD5Final (digest, &context);
						string sz = ob.ResultToHex(digest, true);
						szMD5 = sz.c_str();
					}

					delete buffer;
					buffer = NULL;
				}
			}
			CloseHandle(hFile);			
		}
	}

	szMD5.MakeUpper();
	if(file)
	{
		CString szLog;
		szLog.Format(_T("File(%s) - MD5(%s)."), file, szMD5);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	}
	return szMD5;
}


bool CDownloadManager::SetObserver( IObserver *pObserver )
{
	m_pObserver = pObserver;
	return true;
}
void CDownloadManager::Notify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam)
{
	if(m_pObserver)
	{
		m_pObserver->OnNotify(pdlntfData, wParam);
	}
}

void CDownloadManager::Notify(int nIndex, int nSerialNum, UINT nNotityType, LPVOID lpNotifyData)
{
	t_DownloadNotifyPara DownloadNotifyPara = {0};
	DownloadNotifyPara.nIndex = nIndex;
	DownloadNotifyPara.nNotityType = nNotityType;
	DownloadNotifyPara.lpNotifyData = lpNotifyData;
	DownloadNotifyPara.nSerialNum = nSerialNum;
	Notify(&DownloadNotifyPara, NULL);
}