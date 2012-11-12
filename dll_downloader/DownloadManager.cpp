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


//设置下载的线程数

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
// 下载任务的线程函数
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
	//发送开始下载的消息
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
	//使用PID获取产品信息
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



	//设置下载线程数量，后面需要调用
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

	//添加重试次数
	for(int i = 0; i < RETRY_TIME; ++i)
	{
	// 启动多线程下载任务
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
			// 等待所有线程下载完成
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
	
		//测试
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

		//睡眠N秒后重试
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
// 多线程断点续传下载一个文件
//
BOOL CDownloadManager::Download (
							 LPCTSTR lpszDownloadURL,
							 LPCTSTR lpszSavePath,
							 LPCTSTR lpszSaveOnlyFileName,
							 int iPID,
							 LPCTSTR lpszUsername/*=NULL*/,
							 LPCTSTR lpszPassword/*=NULL*/,
							 BOOL bForceDownload/*=FALSE*/		// 如果为 TRUE 表示强制性重新下载，已下载的部分将会被删除，FALSE 表示断点续传
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

	// 用户提供的URL可能已经经过转义处理EnCode，需要DeCode还原为原来的URL
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

	// 创建取站点信息对象
	if ( !( m_pDownloadPub_Info = CreateDownloadObject (1) ) )
	{
		CLOSE_HANDLE ( m_hEvtEndModule );

		szLog = _T("Create download object failed.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
		NsResultEnum rv = RS_ERR_GET_WEBINFO_FAILED;
		return HandleDownloadFinished(rv);
	}

	m_pDownloadPub_Info->SetTaskID(m_nSerialNum);

	// 设置取站点信息对象的参数
	m_pDownloadPub_Info->SetAuthorization ( lpszUsername, lpszPassword );
	m_pDownloadPub_Info->m_pDownloadMTR = this;
	m_pDownloadPub_Info->SetDownloadUrl ( c_decode_url );
	m_pDownloadPub_Info->SetObserver(m_pObserver);

	// 创建一个下载线程
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
// 创建下载对象
//
CDownloadBase* CDownloadManager::CreateDownloadObject ( int nCount/*=1*/ )
{
	if ( nCount < 1 ) return NULL;
	CDownloadBase *pDownloadPub = NULL;
	if ( m_csProtocolType.CompareNoCase ( _T("http") ) == 0 )
	{
		pDownloadPub = (CDownloadBase*)new CDownloadHttp[nCount];
	}
	//ftp暂时不支持
	//else if ( m_csProtocolType.CompareNoCase ( _T("ftp") ) == 0 )
	//{
	//	pDownloadPub = (CDownloadBase*)new CDownloadFtp[nCount];
	//}
	else return NULL;

	return pDownloadPub;
}

//
// 删除下载对象
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

// nIndex : 线程序号
// nDownloadedSize : 当前线程，累计已经保存到文件的字节数
// nSimpleSaveSize : 当前线程，此次保存到文件的字节数
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

			//回调
			if(IsCallBackEnable())
			{
				Notify ( nIndex, GetSerialNum(), NOTIFY_TYPE_HAS_GOT_THREAD_DOWNLOADED_SIZE, (LPVOID)nDownloadedSize );

				if ( m_pDownloadPub_Info )
				{
					__int64 nTotalUndownloadBytes = 0;
					for ( int nIndex = 0; nIndex < m_nThreadCount; nIndex++ )
					{
						// 注意：此时缓冲区的字节数为0， Get_TempSaveBytes（）不能获取到正确的缓冲区字节数，因为回调后才更新此数据
						nTotalUndownloadBytes += (m_pDownloadPub_MTR[nIndex].Get_WillDownloadSize () - ( m_pDownloadPub_MTR[nIndex].Get_DownloadedSize () + 0 ));
					}
					__int64 nFileTotalSize = Get_FileTotaleSize();
					if ( nFileTotalSize > 0 )
					{
						// 文件大小减去未完成的，就是已下载的
						__int64 nHasDownedSize = nFileTotalSize - nTotalUndownloadBytes;

						//////////////////////////////////////////////////////////////////////////
						NotifyData *pCallBackInfo = new NotifyData;
						pCallBackInfo->nID = GetSerialNum();
						pCallBackInfo->cbCurPos = nHasDownedSize; /*Get_TotalDownloadedSize()*/ /* 此时缓冲区数据未刷新，不能直接使用此函数 */
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

						//最多每秒上报一次
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
	//使用权重，随机数
	if(m_pid_info.servers_.size() > 0)
	{
		//必须是本host存在于servers列表里才替换host
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
// 创建多线程下载使用的对象和数据缓冲
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
	// 设置多线程下载使用的对象的参数
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

	// 创建多线程下载使用的数据缓冲
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
// 删除多线程下载使用的对象和数据缓冲
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
// 删除取站点信息的下载对象
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
// 启动多线程下载，先用m_pDownloadPub_Info获取站点信息，然后再起多个下载线程
//
NsResultEnum CDownloadManager::StartMTRDownload ()
{
	m_dwDownloadStartTime = GetTickCount();

	////////////////////////////////////////////////////////////////////////////
	////发送开始下载的消息
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
	// 先获取站点信息，文件名，长度，md5值
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

	//至此已获得文件名大小等信息
	BOOL bMustCreateNullFile = TRUE;
	if ( m_bForceDownload )
	{
		// 需要重新下载
		::DeleteFile ( m_csSavePathFileName );
		::DeleteFile ( GetTempFilePath() );
	}
	else
	{
		BOOL bNeedReDownLoad = TRUE;
		// 测试目标文件是否存在，若已经存在，根据是否需要校验MD5，分别处理
		HANDLE hFile = ::CreateFile(m_csSavePathFileName,	// file to open
									GENERIC_READ,			// open for read
									FILE_SHARE_READ,		// share for reading
									NULL,					// default security
									OPEN_EXISTING,			// existing file only
									FILE_ATTRIBUTE_NORMAL,	// normal file
									NULL);					// no attr. template
		if (hFile != INVALID_HANDLE_VALUE) 
		{ 
			// 文件存在
			if(IsNeedCheckMD5() && (GetMD5_WebSite() != _T("")))
			{
				// 需要验证MD5
				// 获取MD5
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
				// 不需要验证MD5，简单验证文件大小即可
				// 获取文件大小
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
		//需要下载，先读取下载信息
		szLog = _T("Need download.");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

		// 读取下载信息，如果能成功读到，说明上次下载尚未完成
		if ( m_pDownloadPub_Info->Is_SupportResume() )
		{
			// 测试临时文件是否存在，若已存在，读取下载信息
			HANDLE hFile = ::CreateFile(GetTempFilePath(),						// file to open
				GENERIC_READ,							// open for read
				FILE_SHARE_READ,						// share for reading
				NULL,									// default security
				OPEN_EXISTING,							// existing file only
				FILE_ATTRIBUTE_NORMAL,					// normal file
				NULL);									// no attr. template
			if (hFile != INVALID_HANDLE_VALUE) 
			{ 
				// 获取临时文件大小
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
		// 创建一个用来保存下载数据的空文件
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

	// 设置临时文件属性为隐藏
	::SetFileAttributes ( GetTempFilePath(), FILE_ATTRIBUTE_HIDDEN );

	//////////////////////////////////////////////////////////////////////////
	// 分配下载任务
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
// 得到临时数据保存的路径文件名
//
CString CDownloadManager::GetTempFilePath ()
{
	_ASSERT ( !m_csSavePathFileName.IsEmpty () );
	CString csTempFileName;
	csTempFileName.Format ( _T("%s.~P2S"), m_csSavePathFileName );
	return csTempFileName;
}

//
// 分配下载任务
//
BOOL CDownloadManager::AssignDownloadTask()
{
	_ASSERT ( m_pDownloadPub_Info );

	CString szLog;
	//如果不支持断点续传，则清空重新开始下载
	if ( !m_pDownloadPub_Info->Is_SupportResume() )
	{
		DeleteDownloadObjectAndDataMTR ();

		szLog.Format(_T("The website is not support resume download [%s]."), m_pDownloadPub_Info->Get_ServerName());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
	}
	// 文件大小未知，采用单线程
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

	//如果没有载入下载信息，则需要重新new下载对象
	if ( !DownloadInfoIsValid() || !m_pDownloadPub_MTR || !m_pDownloadCellInfo )
	{
		if ( !CreateDownloadObjectAndDataMTR () )
			return FALSE;
	}

	_ASSERT ( m_pDownloadPub_MTR && m_pDownloadCellInfo );

	// 下载任务尚未分配
	szLog = _T("Assign task:\r\n-------------------->>>");
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	//无效才需要重新赋值
	if ( !DownloadInfoIsValid() )
	{
		//////////////////////////////////////////////////////////////////////////
		//如果有服务器数据，则使用服务器返回的数据进行分配
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
			//使用默认的分配方式
			__int64 nWillDownloadSize = -1, nWillDownloadStartPos = 0, nNoAssignSize = 0;
			if ( m_pDownloadPub_Info->Get_FileTotalSize () > 0 )
			{
				nWillDownloadSize = m_pDownloadPub_Info->Get_FileTotalSize () / m_nThreadCount;
				// 均分后剩下的部分，让第一个线程来承担下载
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


	// 启动下载任务
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
// 从下载信息文件中读取下载信息
//
BOOL CDownloadManager::ReadDownloadInfo()
{
	CString csTempFileName = GetTempFilePath ();
	BOOL bRet = FALSE;

	// 测试文件是否存在
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
// 从下载信息文件中读取下载信息
//
BOOL CDownloadManager::ReadDownloadInfo()
{
	CString csTempFileName = GetTempFilePath ();
	BOOL bRet = FALSE;

	// 测试文件是否存在
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

	// 测试文件是否存在
	HANDLE hFile = ::CreateFile(csTempFileName,	// file to open
		GENERIC_READ | GENERIC_WRITE,			// open for read write
		FILE_SHARE_READ | FILE_SHARE_WRITE,		// share for reading
		NULL,									// default security
		OPEN_EXISTING,							// existing file only
		FILE_ATTRIBUTE_NORMAL,					// normal file
		NULL);									// no attr. template
	if (hFile == INVALID_HANDLE_VALUE) 
	{ 
		// 文件不存在
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

	// 测试文件是否存在
	HANDLE hFile = ::CreateFile(csTempFileName,							// file to open
								GENERIC_READ | GENERIC_WRITE,			// open for read write
								FILE_SHARE_READ | FILE_SHARE_WRITE,		// share for reading
								NULL,									// default security
								OPEN_EXISTING,							// existing file only
								FILE_ATTRIBUTE_NORMAL,					// normal file
								NULL);									// no attr. template
	if (hFile == INVALID_HANDLE_VALUE) 
	{ 
		// 文件不存在
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

		// 重新设置文件长度
		{
			bRet = FALSE;
			// 测试文件是否存在
			HANDLE hFile = ::CreateFile(csTempFileName,							// file to open
										GENERIC_READ | GENERIC_WRITE,			// open for read write
										FILE_SHARE_READ | FILE_SHARE_WRITE,		// share for reading
										NULL,									// default security
										OPEN_EXISTING,							// existing file only
										FILE_ATTRIBUTE_NORMAL,					// normal file
										NULL);									// no attr. template
			if (hFile != INVALID_HANDLE_VALUE) 
			{ 
				// 文件存在
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
			// 将临时文件，改名为目标文件名
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
			// 设置文件属性
			::SetFileAttributes(m_csSavePathFileName, FILE_ATTRIBUTE_NORMAL);

			// 验证MD5
			if(IsNeedCheckMD5() && GetMD5_WebSite() != _T(""))
			{
				bRet = FALSE;
				BOOL bPartRet = TRUE;
				//////////////////////////////////////////////////////////////////////////
				//验证分段md5,只有多于一个分段才检查
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
					// 获取MD5
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
// 下载信息是否有效
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
// 找到剩余未下载的数量最大的那个对象编号, 优先选择未下载成功的那个对象
//
int CDownloadManager::GetUndownloadMaxBytes( __int64 &nUndownloadBytes )
{
	nUndownloadBytes = 0;
	int nMaxIndex = -1;
	for ( int nIndex = 0; nIndex < m_nThreadCount; nIndex++ )
	{
		// 优先选择未下载成功的那个对象
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
// 编号为 nIndex 的对象调度任务，为下载任务最繁重的对象减轻负担
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

	//如果剩余下载数据不到100K，则不需要协助下载
	if ( m_pDownloadPub_MTR[nIndex_Heavy].ThreadIsRunning() && nUndownloadBytes < 100*1024 )
	{
		return FALSE;
	}

	_ASSERT ( nIndex_Heavy >= 0 && nIndex_Heavy < m_nThreadCount );
	_ASSERT ( m_pDownloadPub_MTR[nIndex_Heavy].Get_WillDownloadStartPos() == m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos );

	CString szLog;
	szLog.Format(_T("Download thread(%d) help thread(id:%d status:%s)."), nIndex, nIndex_Heavy, m_pDownloadPub_MTR[nIndex_Heavy].ThreadIsRunning()?_T("running"):_T("stopped"));
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	// 给空闲下载对象分配新任务
	m_pDownloadCellInfo[nIndex].nWillDownloadSize = ( m_pDownloadPub_MTR[nIndex_Heavy].ThreadIsRunning()?(nUndownloadBytes/2) : nUndownloadBytes );
	m_pDownloadCellInfo[nIndex].nWillDownloadStartPos = m_pDownloadPub_MTR[nIndex_Heavy].Get_WillDownloadStartPos() +
		m_pDownloadPub_MTR[nIndex_Heavy].Get_WillDownloadSize() - m_pDownloadCellInfo[nIndex].nWillDownloadSize;
	m_pDownloadCellInfo[nIndex].nDownloadedSize = 0;

	szLog.Format(_T("Free download thread(%d) assign new task: from %I64d(0x%I64x) to %I64d(0x%I64x), Totol %I64d(0x%I64x) bytes."), nIndex, m_pDownloadCellInfo[nIndex].nWillDownloadStartPos, m_pDownloadCellInfo[nIndex].nWillDownloadStartPos,
		m_pDownloadCellInfo[nIndex].nWillDownloadStartPos + m_pDownloadCellInfo[nIndex].nWillDownloadSize, m_pDownloadCellInfo[nIndex].nWillDownloadStartPos + m_pDownloadCellInfo[nIndex].nWillDownloadSize,
		m_pDownloadCellInfo[nIndex].nWillDownloadSize, m_pDownloadCellInfo[nIndex].nWillDownloadSize);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);


	// 启动空闲下载对象的下载任务
	m_pDownloadPub_MTR[nIndex].ResetVar ();
	if ( !m_pDownloadPub_MTR[nIndex].Download ( m_pDownloadCellInfo[nIndex].nWillDownloadStartPos,
		m_pDownloadCellInfo[nIndex].nWillDownloadSize, m_pDownloadCellInfo[nIndex].nDownloadedSize ) )
	{
		return FALSE;
	}

	// 减轻繁忙下载对象的任务
	m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize -= m_pDownloadCellInfo[nIndex].nWillDownloadSize;
	m_pDownloadPub_MTR[nIndex_Heavy].Set_WillDownloadSize ( m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize );

	szLog.Format(_T("Download thread(%d) has downloaded %I64d(0x%I64x), Remaining %I64d(0x%I64x) bytes; Adjust task as: from %I64d(0x%I64x) to %I64d(0x%I64x), Totol %I64d(0x%I64x) bytes."), nIndex_Heavy, m_pDownloadPub_MTR[nIndex_Heavy].Get_DownloadedSize(), m_pDownloadPub_MTR[nIndex_Heavy].Get_DownloadedSize(),
		nUndownloadBytes, nUndownloadBytes,
		m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos, m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos,
		m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos + m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize, m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadStartPos + m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize,
		m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize, m_pDownloadCellInfo[nIndex_Heavy].nWillDownloadSize);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	// 因nIndex_Heavy线程对象的任务被调度给了nIndex，若nIndex_Heavy本身已经退出了，则需要将其下载标志设置为成功
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
// 获取下载所消耗的时间（毫秒），可用来计算下载速度和推算剩余时间
//
DWORD CDownloadManager::GetDownloadElapsedTime()
{
	return (GetTickCount() - m_dwDownloadStartTime);
}

//
// 停止下载。将所有下载线程关闭，将下载对象删除，文件关闭
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

	// 如果用户指定了新的保存文件名，就用新的。
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
		//如果后缀名为空则为空
		if (wcslen(szExtensionName_User) < 1)
		{
			m_csSavePathFileName.Format ( _T("%s%s"), StandardizationFileForPathName(m_csSavePath,FALSE),
				StandardizationFileForPathName(szOnlyFileName_NoExt_User,TRUE));
		}
		//拷贝后缀名
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
// 根据 URL 来获取本地保存的文件名
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
// 获取文件大小
//
__int64 CDownloadManager::Get_FileTotaleSize()
{
	if ( !m_pDownloadPub_Info ) return -1;
	return m_pDownloadPub_Info->Get_FileTotalSize ();
}

//
// 获取已下载的字节数，包括以前下载的和本次下载的
//
__int64 CDownloadManager::Get_TotalDownloadedSize()
{
	//如果已经完成则直接返回文件大小 lr
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

	// 文件大小减去未完成的，就是已下载的
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

//创建新的结束事件
//1  ->成功创建
//0  ->原下载正在进行
//-1 ->创建失败
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
		//句柄有效，原下载正在进行
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
// 等待下载结束
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

			//如果只剩下主线程
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
				// 某下载线程退出了，且线程序号在正常范围
				int nIndex = FindIndexByThreadHandle ( lpHandles[nRet] );
				if ( ( nIndex >= 0 && nIndex < m_nThreadCount ) )
				{
					// 找到了这个线程的编号
					if(m_pDownloadPub_MTR[nIndex].Is_DownloadSuccess())
					{
						// 此线程已下载成功
						// 尝试帮助任务最重的线程，若失败，则此线程任务关闭
						if ( !AttemperDownloadTask ( nIndex ) )
						{
							m_pDownloadPub_MTR[nIndex].Clear_Thread_Handle ();
						}
					}
					else
					{
						// 此线程未下载成功
						// 关闭此任务，等待其它线程帮助其完成任务
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
				// 用户选择停止任务 m_hEvtEndModule有信号
				CString szLog;
				szLog.Format(_T("User canceled while downloading, %s"), m_csDownloadURL);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

				eDownloadResult = RS_ERR_USER_CANCEL;
				break;
			}
			// 模块结束		
			else
			{
				// 本次调用失败，重复此过程
				eDownloadResult = RS_ERR_UNKOWN;
			}
		}
	}
	

	// 有异常，等待所有下载线程结束
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


// 设置是否需要验证MD5
void CDownloadManager::SetNeedCheckMD5(BOOL bCheck)
{
	m_bNeedCheckMD5 = bCheck;
}

// 是否需要验证MD5
BOOL CDownloadManager::IsNeedCheckMD5()
{
	return m_bNeedCheckMD5;
}

// 从站点获取到的MD5值
CString CDownloadManager::GetMD5_WebSite()
{
	return m_csMD5_Web;
}

// 设置要下载文件的MD5值，此为基准值
void CDownloadManager::SetMD5_WebSite(CString szMD5)
{
	m_csMD5_Web = szMD5;
	m_csMD5_Web.MakeUpper();
	CString szLog;
	szLog.Format(_T("SetMD5_WebSite: [") + m_csMD5_Web + _T("] in function SetMD5_WebSite"));
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);
}

// 获取某个文件的MD5值
CString CDownloadManager::GetPartFileMD5(wchar_t *file, __int64 offset, __int64 length)
{
	CString szMD5;
	if(file)
	{
		// 测试文件是否存在
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
				// 循环读文件
				int nKit = 2*1024*1024; // 64M ***必须为64的整数倍
				char *buffer = new char[nKit];
				if(buffer)
				{
					memset(buffer, 0, nKit);

					// 初始化MD5类对象
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

// 获取某个文件的MD5值
CString CDownloadManager::GetFileMD5(wchar_t *file)
{
	CString szMD5;
	if(file)
	{
		// 测试文件是否存在
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
				// 循环读文件
				int nKit = 64*1024*1024; // 64M ***必须为64的整数倍
				char *buffer = new char[nKit];
				if(buffer)
				{
					memset(buffer, 0, nKit);

					// 初始化MD5类对象
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