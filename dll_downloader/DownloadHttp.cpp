/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:25
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\DownloadHttp.cpp
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	DownloadHttp
	file ext:	cpp
	author:		tanyc
	
	purpose:	
*********************************************************************/

#include "stdafx.h"
#include "DownloadHttp.h"
#include <shlwapi.h>
#include "DownloadManager.h"

CDownloadHttp::CDownloadHttp()
{
}

CDownloadHttp::~CDownloadHttp()
{
}

BOOL CDownloadHttp::DownloadOnce()
{
	__int64 nWillDownloadSize = Get_WillDownloadSize();				// ����Ӧ�����ص��ֽ���
	__int64 nDownloadedSize = Get_DownloadedSize ();				// �������ֽ���
	if ( nWillDownloadSize > 0 && nDownloadedSize >= nWillDownloadSize )
	{
		//return DownloadEnd(TRUE);
		return TRUE;
	}
	if ( !CDownloadBase::DownloadOnce () )
	{
		//return DownloadEnd(FALSE);
		return FALSE;
	}
	//��������http header��ʱ�����յ����ݣ�\r\n\r\n���棩
	char szTailData[NET_BUFFER_SIZE] = {0};
	int nTailSize = sizeof(szTailData);

	//��ȡhttp header��Ϣ
	if ( !RequestHttpData ( TRUE, szTailData, &nTailSize ) )
	{
		//return DownloadEnd(FALSE);
		return FALSE;
	}
	// ��HTTP�������ж�ȡ���ݣ������浽�ļ���
	//return DownloadEnd ( RecvDataAndSaveToFile(m_SocketClient,szTailData, nTailSize) );
	return RecvDataAndSaveToFile(m_SocketClient,szTailData, nTailSize);
}

BOOL CDownloadHttp::RequestHttpData(BOOL bGet, char *szTailData/*=NULL*/, int *pnTailSize/*=NULL*/ )
{
	int nTailSizeTemp = 0;
	BOOL bRetryRequest = TRUE;
	BOOL bContentError = FALSE;
	while ( bRetryRequest )
	{
		CString csReq = GetRequestStr ( bGet );
		CString csResponse;
		nTailSizeTemp = pnTailSize?(*pnTailSize):0;
		if ( !SendRequest ( csReq, csResponse, szTailData, &nTailSizeTemp ) )
			return FALSE;

		/*
		CString csReferer_Old = m_csReferer;
		CString csDownloadUrl_Old = m_csDownloadUrl;
		CString csServer_Old = m_csServer;
		CString csObject_Old = m_csObject;
		USHORT nPort_Old = m_nPort;
		CString csProtocolType_Old = m_csProtocolType;
		*/
		if ( !ParseResponseString ( csResponse, bRetryRequest, bContentError) )
		{
			return FALSE;
			/*
			if (bContentError)
			{
				bRetryRequest = FALSE;
				return FALSE;
			}

			if ( !m_csCookieFlag.IsEmpty () )
			{
				m_csCookieFlag.Empty();
				return FALSE;
			}
			m_csReferer = csReferer_Old;
			m_csDownloadUrl = csDownloadUrl_Old;
			m_csServer = csServer_Old;
			m_csObject = csObject_Old;
			m_nPort = nPort_Old;
			m_csProtocolType = csProtocolType_Old;
			m_csCookieFlag = "Flag=UUIISPoweredByUUSoft";
			bRetryRequest = TRUE;
			*/
		}
	}
	if ( pnTailSize )
		*pnTailSize = nTailSizeTemp;

	return TRUE;
}

//
// ��ȡԶ��վ����Ϣ���磺�Ƿ�֧�ֶϵ�������Ҫ���ص��ļ���С�ʹ���ʱ���
//
BOOL CDownloadHttp::GetRemoteSiteInfo_Pro()
{
	if ( !CDownloadBase::GetRemoteSiteInfo_Pro() )
	{
		return FALSE;
	}
	if ( !RequestHttpData ( TRUE ) )
	{
		return FALSE;
	}

	//����Ƿ�֧�ֶϵ�����
	CheckIfSupportResume();

	return TRUE;
}

BOOL CDownloadHttp::CheckIfSupportResume()
{
	//���֧�ֶϵ������Ͳ���Ҫ���ж�
	if(m_bSupportResume)
	{
		CString szLog;
		szLog.Format(_T("already support Resume: %s"), m_csDownloadUrl);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return TRUE;
	}

	char *szTailData = NULL;
	int nTailSizeTemp = 0;
	BOOL bRetryRequest = TRUE;
	BOOL bContentError = FALSE;
	while ( bRetryRequest )
	{
		CString csReq = GetRequestStrForResume ( true );
		CString csResponse;
		nTailSizeTemp = 0;
		if ( !SendRequest( csReq, csResponse, szTailData, &nTailSizeTemp ) )
			return FALSE;
		if ( !ParseResponseStringForResume ( csResponse, bRetryRequest, bContentError) )
		{
			return FALSE;
		}
	}

	return TRUE;
}

CString CDownloadHttp::GetRequestStrForResume(BOOL bGet)
{
	CString strVerb;
	if( bGet )
		strVerb = _T("GET ");
	else
		strVerb = _T("HEAD ");

	CString csReq, strAuth, strRange;
	csReq  = strVerb  + m_csObject + _T(" HTTP/1.1\r\n");

	if ( !m_csUsername.IsEmpty () )
	{
		strAuth = _T("");
		Base64Encode ( m_csUsername + _T(":") + m_csPassword, strAuth );
		csReq += _T("Authorization: Basic ") + strAuth + _T("\r\n");
	}

	csReq = csReq + _T("Accept: */*\r\n");
	if( m_csReferer.IsEmpty() )
	{
		m_csReferer = GetRefererFromURL ();
	}
	csReq = csReq + _T("Referer: ")+m_csReferer+_T("\r\n");
	csReq = csReq + _T("User-Agent: ") + m_csUserAgent + _T("\r\n");

	CString csPort;
	if ( m_nPort != DEFAULT_HTTP_PORT )
		csPort.Format ( _T(":%d"), m_nPort );
	csReq = csReq + _T("Host: ") + m_csServer + csPort + _T("\r\n");


	//	csReq = csReq + _T("Pragma: no-cache\r\n"); 
	//	csReq = csReq + _T("Cache-Control: no-cache\r\n");


	//csReq = csReq + _T("Accept-Encoding: gzip,deflate") + _T("\r\n");
	//csReq = csReq + _T("Content-Length: 0") + _T("\r\n");

	csReq = csReq + _T("Connection: Close\r\n");

	if ( !m_csCookieFlag.IsEmpty() )
	{
		csReq = csReq + _T("Cookie: ") + m_csCookieFlag + _T("\r\n");
	}

	// ָ��Ҫ���ص��ļ���Χ,ֻΪ���ж��Ƿ񷵻�206
	strRange.Format ( _T("Range: bytes=0-1\r\n") );

	csReq = csReq + strRange;
	csReq = csReq + _T("\r\n");

	CString szLog;
	szLog.Format(_T("Sub download thread(%d), http GetRequestStrForResume is:\r\n%s."), m_nIndex, csReq);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	return csReq;
}

BOOL CDownloadHttp::ParseResponseStringForResume( CString csResponseString, OUT BOOL &bRetryRequest , OUT BOOL &bContError)
{
	CString szLog;
	bRetryRequest = FALSE;

	// ��ȡ���ش���
	CString csOneLine = GetOneLine ( csResponseString );
	csResponseString.ReleaseBuffer();
	DWORD dwResponseCode = GetResponseCode ( csOneLine );
	szLog.Format(_T("Sub download thread(%d), Received response code:%d."), m_nIndex, dwResponseCode);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);
	if ( dwResponseCode < 1 )
	{
		return FALSE;
	}

	int nPos = 0;
	// �����ļ����ض���
	if( dwResponseCode >= 300 && dwResponseCode < 400 )
	{
		bRetryRequest = TRUE;
		// �õ������ļ��µ�URL
		CString csRedirectFileName = FindAfterFlagString ( _T("location:"), csResponseString );

		{
			// ����ļ��ض����url��ת�壬�������ӵ���url��ַʱ����ȡ��������Ӧ�쳣��Location:��ǩָ����µ�ַ����
			csRedirectFileName.Replace(_T("%3A"), _T(":"));
			csRedirectFileName.Replace(_T("%2F"), _T("/"));
		}

		// ���� Referer
		m_csReferer = GetRefererFromURL ();

		// �ض��������ķ�����
		nPos = csRedirectFileName.Find(_T("://"));
		if ( nPos >= 0 )
		{
			m_csDownloadUrl = csRedirectFileName;
			// ����Ҫ���ص�URL�Ƿ���Ч
			if ( !ParseURLDIY ( m_csDownloadUrl, m_csServer, m_csObject, m_nPort, m_csProtocolType ) )
			{
				szLog.Format(_T("Sub download thread(%d), Redirect media path [%s] invalid."), m_nIndex, m_csDownloadUrl);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);

				return FALSE;
			}
			return TRUE;
		}

		// �ض��򵽱��������������ط�
		csRedirectFileName.Replace ( _T("\\"), _T("/") );
		// �ض����ڸ�Ŀ¼
		if( csRedirectFileName[0] == '/' )
		{
			m_csObject = csRedirectFileName;
			return TRUE;
		}

		// ��������Ե�ǰĿ¼
		int nParentDirCount = 0;
		nPos = csRedirectFileName.Find ( _T("../") );
		while ( nPos >= 0 )
		{
			csRedirectFileName = csRedirectFileName.Mid(nPos+3);
			nParentDirCount++;
			nPos = csRedirectFileName.Find(_T("../"));
		}
		for (int i=0; i<=nParentDirCount; i++)
		{
			nPos = m_csDownloadUrl.ReverseFind('/');
			if (nPos != -1)
				m_csDownloadUrl = m_csDownloadUrl.Left(nPos);
		}
		if ( csRedirectFileName.Find ( _T("./"), 0 ) == 0 )
			csRedirectFileName.Delete ( 0, 2 );
		m_csDownloadUrl = m_csDownloadUrl+_T("/")+csRedirectFileName;

		return ParseURLDIY ( m_csDownloadUrl, m_csServer, m_csObject, m_nPort, m_csProtocolType );
	}
	// ���󱻳ɹ����ա����ͽ���
	else if( dwResponseCode >= 200 && dwResponseCode < 300 )
	{
		//////////////////////////////////////////////////////////////////////////
		if ( dwResponseCode == 206 )	// ֧�ֶϵ�����
		{

			szLog.Format(_T("Sub download thread(%d), set support resume true."), m_nIndex);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
			m_bSupportResume = TRUE;
		}
		else							// ��֧�ֶϵ�����
		{
			m_bSupportResume = FALSE;
		}
		return TRUE;
	}

	szLog.Format(_T("Sub download thread(%d), Receive invalid code : %d."), m_nIndex, dwResponseCode);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
	return FALSE;
}

CString CDownloadHttp::GetRequestStr(BOOL bGet)
{
	CString strVerb;
	if( bGet )
		strVerb = _T("GET ");
	else
		strVerb = _T("HEAD ");

	CString csReq, strAuth, strRange;
	csReq  = strVerb  + m_csObject + _T(" HTTP/1.1\r\n");

	if ( !m_csUsername.IsEmpty () )
	{
		strAuth = _T("");
		Base64Encode ( m_csUsername + _T(":") + m_csPassword, strAuth );
		csReq += _T("Authorization: Basic ") + strAuth + _T("\r\n");
	}

	csReq = csReq + _T("Accept: */*\r\n");
	if( m_csReferer.IsEmpty() )
	{
		m_csReferer = GetRefererFromURL ();
	}
	csReq = csReq + _T("Referer: ")+m_csReferer+_T("\r\n");
	csReq = csReq + _T("User-Agent: ") + m_csUserAgent + _T("\r\n");

	CString csPort;
	if ( m_nPort != DEFAULT_HTTP_PORT )
		csPort.Format ( _T(":%d"), m_nPort );
	csReq = csReq + _T("Host: ") + m_csServer + csPort + _T("\r\n");


	//	csReq = csReq + _T("Pragma: no-cache\r\n"); 
	//	csReq = csReq + _T("Cache-Control: no-cache\r\n");


	//csReq = csReq + _T("Accept-Encoding: gzip,deflate") + _T("\r\n");
	//csReq = csReq + _T("Content-Length: 0") + _T("\r\n");

	csReq = csReq + _T("Connection: Close\r\n");

	if ( !m_csCookieFlag.IsEmpty() )
	{
		csReq = csReq + _T("Cookie: ") + m_csCookieFlag + _T("\r\n");
	}

	// ָ��Ҫ���ص��ļ���Χ
	CString csEndPos;
	__int64 nWillDownloadStartPos = Get_WillDownloadStartPos ();	// ��ʼλ��
	__int64 nWillDownloadSize = Get_WillDownloadSize();				// ����Ӧ�����ص��ֽ���
	__int64 nDownloadedSize = Get_DownloadedSize ();				// �������ֽ���
	if ( nWillDownloadSize > 0 )
		csEndPos.Format ( _T("%I64d"), nWillDownloadStartPos+nWillDownloadSize-1 );
	_ASSERT ( nWillDownloadSize <= 0 || nDownloadedSize <= nWillDownloadSize );
	strRange.Format ( _T("Range: bytes=%I64d-%s\r\n"), nWillDownloadStartPos+nDownloadedSize, csEndPos );

	csReq = csReq + strRange;
	csReq = csReq + _T("\r\n");

	CString szLog;
	szLog.Format(_T("Sub download thread(%d), http get request is:\r\n%s."), m_nIndex, csReq);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	return csReq;
}

//
// ��������ύ���󣬲��õ������ַ���
//
BOOL CDownloadHttp::SendRequest(LPCTSTR lpszReq, CString &csResponse, char *szTailData/*=NULL*/, int *pnTailSize/*=NULL*/ )
{
	CString szLog;
	m_SocketClient.Disconnect ();
	if ( !Connect () ) return FALSE;
	if ( !m_SocketClient.SendString2 ( lpszReq ) )
	{
		return FALSE;
	}

	for ( int i=0; ; i++ )
	{
		char szRecvBuf[NET_BUFFER_SIZE] = {0};
		int nReadSize = m_SocketClient.Receive ( szRecvBuf, sizeof(szRecvBuf) );
		if ( nReadSize <= 0 )
		{
			szLog.Format(_T("Sub download thread(%d), Receive response data failed."), m_nIndex);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);

			return FALSE;
		}
		csResponse += szRecvBuf;
		char *p = strstr ( szRecvBuf, "\r\n\r\n" );
		if ( p )
		{
			if ( szTailData && pnTailSize && *pnTailSize > 0 )
			{
				p += 4;
				int nOtioseSize = nReadSize - int( p - szRecvBuf );
				*pnTailSize = MIN ( nOtioseSize, *pnTailSize );
				memcpy ( szTailData, p, *pnTailSize );
			}

			{
				int nPos = csResponse.Find ( _T("\r\n\r\n"), 0 );
				CString csDump;
				if ( nPos >= 0 ) csDump = csResponse.Left ( nPos );
				else csDump = csResponse;

				szLog.Format(_T("Sub download thread(%d), http server response:\r\n------------------------>>>\r\n%s\r\n<<<------------------------"), m_nIndex, csDump);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);
			}
			break;
		}
	}

	return TRUE;
}

DWORD CDownloadHttp::GetResponseCode(CString csLineText)
{
	csLineText.MakeLower ();
	_ASSERT ( csLineText.Find ( _T("http/"), 0 ) >= 0 );
	int nPos = csLineText.Find ( _T(" "), 0 );
	if ( nPos < 0 ) return 0;
	CString csCode = csLineText.Mid ( nPos + 1 );
	csCode.TrimLeft(); csCode.TrimRight();
	nPos = csCode.Find ( _T(" "), 0 );
	if ( nPos < 0 ) nPos = csCode.GetLength() - 1;
	csCode = csCode.Left ( nPos );

	//������������"HTTP/1.1 200 OK" ����206���� 12.10.25
	//��Ϊ��������������"HTTP/1.0 200 OK"�������ǵķ�������֧�ֶϵ�����������ֱ������200Ϊ206
	//if(csCode == _T("200"))
	//{
	//	return 206;
	//}

	return (DWORD)_wtoi(csCode);
}

BOOL CDownloadHttp::ParseResponseString ( CString csResponseString, OUT BOOL &bRetryRequest , OUT BOOL &bContError)
{
	CString szLog;
	bRetryRequest = FALSE;

	// ��ȡ���ش���
	CString csOneLine = GetOneLine ( csResponseString );
	csResponseString.ReleaseBuffer();
	DWORD dwResponseCode = GetResponseCode ( csOneLine );
	szLog.Format(_T("Sub download thread(%d), Received response code:%d."), m_nIndex, dwResponseCode);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);
	if ( dwResponseCode < 1 )
	{
		return FALSE;
	}

	int nPos = 0;
	// �����ļ����ض���
	if( dwResponseCode >= 300 && dwResponseCode < 400 )
	{
		bRetryRequest = TRUE;
		// �õ������ļ��µ�URL
		CString csRedirectFileName = FindAfterFlagString ( _T("location:"), csResponseString );

		{
			// ����ļ��ض����url��ת�壬�������ӵ���url��ַʱ����ȡ��������Ӧ�쳣��Location:��ǩָ����µ�ַ����
			csRedirectFileName.Replace(_T("%3A"), _T(":"));
			csRedirectFileName.Replace(_T("%2F"), _T("/"));
		}

		// ���� Referer
		m_csReferer = GetRefererFromURL ();

		// �ض��������ķ�����
		nPos = csRedirectFileName.Find(_T("://"));
		if ( nPos >= 0 )
		{
			m_csDownloadUrl = csRedirectFileName;
			// ����Ҫ���ص�URL�Ƿ���Ч
			if ( !ParseURLDIY ( m_csDownloadUrl, m_csServer, m_csObject, m_nPort, m_csProtocolType ) )
			{
				szLog.Format(_T("Sub download thread(%d), Redirect media path [%s] invalid."), m_nIndex, m_csDownloadUrl);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);

				return FALSE;
			}
			return TRUE;
		}

		// �ض��򵽱��������������ط�
		csRedirectFileName.Replace ( _T("\\"), _T("/") );
		// �ض����ڸ�Ŀ¼
		if( csRedirectFileName[0] == '/' )
		{
			m_csObject = csRedirectFileName;
			return TRUE;
		}

		// ��������Ե�ǰĿ¼
		int nParentDirCount = 0;
		nPos = csRedirectFileName.Find ( _T("../") );
		while ( nPos >= 0 )
		{
			csRedirectFileName = csRedirectFileName.Mid(nPos+3);
			nParentDirCount++;
			nPos = csRedirectFileName.Find(_T("../"));
		}
		for (int i=0; i<=nParentDirCount; i++)
		{
			nPos = m_csDownloadUrl.ReverseFind('/');
			if (nPos != -1)
				m_csDownloadUrl = m_csDownloadUrl.Left(nPos);
		}
		if ( csRedirectFileName.Find ( _T("./"), 0 ) == 0 )
			csRedirectFileName.Delete ( 0, 2 );
		m_csDownloadUrl = m_csDownloadUrl+_T("/")+csRedirectFileName;

		return ParseURLDIY ( m_csDownloadUrl, m_csServer, m_csObject, m_nPort, m_csProtocolType );
	}
	// ���󱻳ɹ����ա����ͽ���
	else if( dwResponseCode >= 200 && dwResponseCode < 300 )
	{
		//////////////////////////////////////////////////////////////////////////
		int nSerialNum = GetSerialNum();

		// ��ȡ Content-Type
		CString csDownloadType = FindAfterFlagString ( _T("content-type:"), csResponseString );
		if (csDownloadType.Find(_T("error")) != -1)
		{
			bContError = TRUE;
			return FALSE;
		}
		//////////////////////////////////////////////////////////////////////////

		if ( m_nIndex == -1 )	// ���̲߳���Ҫ��ȡ�ļ���С����Ϣ
		{
			// ��ȡMD5
			{
				CString csMD5_Encode = FindAfterFlagString ( _T("Content-MD5:"), csResponseString );
				if(!csMD5_Encode.IsEmpty())
				{
					szLog.Format(_T("Sub download thread(%d), Content-MD5 encode:(%s)."), m_nIndex, csMD5_Encode);
					__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);


					CString csMD5_Decode;
					if(Base64Decode(csMD5_Encode.GetBuffer(csMD5_Encode.GetLength()), csMD5_Decode))
					{
						m_csMD5_Web = csMD5_Decode;
						if(m_pDownloadMTR != NULL)
						{
							CDownloadManager *pMtr = (CDownloadManager*)m_pDownloadMTR;
							pMtr->SetMD5_WebSite(m_csMD5_Web);
						}
					}
				}
				else
				{
					m_csMD5_Web = _T("");
				}

				szLog.Format(_T("Sub download thread(%d), Content-MD5 decode:(%s)."), m_nIndex, m_csMD5_Web);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
			}


			// ��ȡ Content-Length
			CString csDownFileLen = FindAfterFlagString ( _T("content-length:"), csResponseString );
			m_nFileTotalSize = (__int64) _wtoi64( csDownFileLen.GetBuffer(csDownFileLen.GetLength()) );
			csDownFileLen.ReleaseBuffer();
			//////////////////////////////////////////////////////////////////////////
			if(IsCallBackEnable())
			{
				CString strDownLoadFileName = GetDownloadObjectFileName();
				int nSerialNum = GetSerialNum();
				Notify ( -1, nSerialNum, NOTIFY_TYPE_HAS_GOT_REMOTE_FILENAME, (LPVOID)(strDownLoadFileName.GetBuffer(strDownLoadFileName.GetLength())) );
				strDownLoadFileName.ReleaseBuffer();

				Notify ( -1, nSerialNum, NOTIFY_TYPE_HAS_GOT_REMOTE_FILESIZE, (LPVOID)m_nFileTotalSize );

			}			
			//////////////////////////////////////////////////////////////////////////
			/*
			int nWillDownloadStartPos = Get_WillDownloadStartPos ();	// ��ʼλ��
			int nWillDownloadSize = Get_WillDownloadSize();				// ����Ӧ�����ص��ֽ���
			int nDownloadedSize = Get_DownloadedSize ();				// �������ֽ���
			*/
			/*
			if ( m_nFileTotalSize > 0 && nWillDownloadSize-nDownloadedSize > m_nFileTotalSize )
			Set_WillDownloadSize ( m_nFileTotalSize-nDownloadedSize );
			*/
		}

		// ��ȡ�������ļ�������޸�ʱ��
		CString csModifiedTime = FindAfterFlagString ( _T("last-modified:"), csResponseString );
		if ( !csModifiedTime.IsEmpty() )
		{
			m_TimeLastModified = ConvertHttpTimeString(csModifiedTime);
		}

		if ( dwResponseCode == 206 )	// ֧�ֶϵ�����
		{
			m_bSupportResume = TRUE;
		}
		else							// ��֧�ֶϵ�����
		{
			m_bSupportResume = FALSE;
		}
		return TRUE;
	}

	szLog.Format(_T("Sub download thread(%d), Receive invalid code : %d."), m_nIndex, dwResponseCode);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
	return FALSE;
}

CString CDownloadHttp::FindAfterFlagString(LPCTSTR lpszFoundStr, CString csOrg)
{
	_ASSERT ( lpszFoundStr && wcslen(lpszFoundStr) > 0 );
	CString csReturing, csFoundStr = GET_SAFE_STRING(lpszFoundStr);
	csFoundStr.MakeLower ();

	wchar_t *pTmp = new wchar_t[csOrg.GetLength() + 1];
	wcscpy(pTmp, csOrg.GetBuffer(csOrg.GetLength()));
	csOrg.ReleaseBuffer();
	MytoLower(pTmp);
	CString csOrgLower(pTmp);
	int nPos = csOrgLower.Find ( csFoundStr );
	if ( nPos < 0 ) return _T("");
	csReturing = csOrg.Mid ( nPos + csFoundStr.GetLength() );
	nPos = csReturing.Find(_T("\r\n"));
	if ( nPos < 0 ) return _T("");
	csReturing = csReturing.Left(nPos);
	csReturing.TrimLeft();
	csReturing.TrimRight();

	delete pTmp;

	return csReturing;
}

//
// �� HTTP ��������ʾ��ʱ��ת��Ϊ CTime ��ʽ���磺Wed, 16 May 2007 14:29:53 GMT
//
CTime CDownloadHttp::ConvertHttpTimeString(CString csTimeGMT)
{
	CString csYear, csMonth, csDay, csTime;
	CTime tReturning = -1;
	int nPos = csTimeGMT.Find ( _T(","), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );
	csTimeGMT.TrimLeft(); csTimeGMT.TrimRight ();

	// ��
	nPos = csTimeGMT.Find ( _T(" "), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csDay = csTimeGMT.Left ( nPos );
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );
	csTimeGMT.TrimLeft(); csTimeGMT.TrimRight ();

	// ��
	nPos = csTimeGMT.Find ( _T(" "), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csMonth = csTimeGMT.Left ( nPos );
	int nMonth = GetMouthByShortStr ( csMonth );
	_ASSERT ( nMonth >= 1 && nMonth <= 12 );
	csMonth.Format ( _T("%02d"), nMonth );
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );
	csTimeGMT.TrimLeft(); csTimeGMT.TrimRight ();

	// ��
	nPos = csTimeGMT.Find ( _T(" "), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csYear = csTimeGMT.Left ( nPos );
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );
	csTimeGMT.TrimLeft(); csTimeGMT.TrimRight ();

	// ʱ��
	nPos = csTimeGMT.Find ( _T(" "), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csTime = csTimeGMT.Left ( nPos );
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );

	CString csFileTimeInfo;
	csFileTimeInfo.Format ( _T("%s-%s-%s %s"), csYear, csMonth, csDay, csTime );
	ConvertStrToCTime ( csFileTimeInfo.GetBuffer(csFileTimeInfo.GetLength()), tReturning );
	csFileTimeInfo.ReleaseBuffer();
	return tReturning;
}