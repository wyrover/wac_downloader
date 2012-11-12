#include "StdAfx.h"
#include "ChttpRequest.h"
#include "PublicFunction.h"
#include <iostream>
#include "Base64.hpp"

ChttpRequest::ChttpRequest(void)
{
}


ChttpRequest::~ChttpRequest(void)
{
}

int ChttpRequest::Get(const std::wstring& url, wchar_t* content, int& size)
{
	wchar_t c_decode_url[1024] = {0};
	URL_Decode(url.c_str(), c_decode_url);
	m_Url = url;

	//varify
	if ( !ParseHttpUrl ( c_decode_url ) )
	{
		return -1;
	}

	////connect
	//if(!m_SocketClient.Connect(m_host.c_str(), m_port))
	//{
	//	return -1;
	//}

	BOOL bRetryRequest = TRUE;
	//保存请求http header的时候多接收的数据（\r\n\r\n后面）
	char szTailData[NET_BUFFER_SIZE] = {0};
	int nTailSize = sizeof(szTailData);
	CString csResponse;

	while ( bRetryRequest )
	{
		CString csReq = GetRequestStr ( _T("get") );
		if ( !SendRequest ( csReq, csResponse, szTailData, &nTailSize ) )
			return -1;

		if ( !ParseResponse( csResponse, bRetryRequest) )
		{
			return -1;
		}
	}

	if(!RecvData(csResponse, szTailData, nTailSize))
	{
		return -1;
	}

	wcscpy_s(content, size, csResponse.GetBuffer(csResponse.GetLength()));
	size = csResponse.GetLength();
	return 0;
}

int ChttpRequest::Post(const std::wstring& url, const wchar_t* post_data, wchar_t* content, int& size)
{
	wchar_t c_decode_url[1024] = {0};
	URL_Decode(url.c_str(), c_decode_url);
	m_Url = url;

	//varify
	if ( !ParseHttpUrl ( c_decode_url ) )
	{
		return -1;
	}

	//connect
	if(!m_SocketClient.Connect(m_host.c_str(), m_port))
	{
		return -1;
	}

	BOOL bRetryRequest = TRUE;
	//保存请求http header的时候多接收的数据（\r\n\r\n后面）
	char szTailData[NET_BUFFER_SIZE] = {0};
	int nTailSize = sizeof(szTailData);
	CString csResponse;

	while ( bRetryRequest )
	{
		CString csReq = GetRequestStr ( _T("post"), post_data );
		if ( !SendRequest ( csReq, csResponse, szTailData, &nTailSize ) )
			return -1;

		if ( !ParseResponse( csResponse, bRetryRequest) )
		{
			return -1;
		}
	}

	if(!RecvData(csResponse, szTailData, nTailSize))
	{
		return -1;
	}

	wcscpy_s(content, size, csResponse.GetBuffer(csResponse.GetLength()));
	size = csResponse.GetLength();
	return 0;
}

void ChttpRequest::url_Encode(std::wstring& str)
{
	std::wstring output;
	int size = str.length();
	for(int i = 0; i < size; ++i)
	{
		if(str[i] == _T(' '))
		{
			output += _T("%20");
		}
		else if(str[i] == _T('+'))
		{
			output += _T("%2B");
		}
		else
		{
			output += str[i];
		}
	}

	str = output;
}
// 从URL里面拆分出Server、Object、协议类型等信息，其中 Object 里的值是区分大小写的，否则有些网站可能会下载不了
//
BOOL ChttpRequest::ParseHttpUrl(LPCTSTR lpszURL)
{
	if ( !lpszURL || wcslen(lpszURL) < 1 ) return FALSE;
	CString csURL_Lower(lpszURL);
	csURL_Lower.TrimLeft();
	csURL_Lower.TrimRight();
	csURL_Lower.Replace ( _T("\\"), _T("/") );
	CString csURL = csURL_Lower;
	csURL_Lower.MakeLower ();

	// 清除数据
	m_host = _T("");
	m_object = _T("");
	m_port	  = 0;

	int nPos = csURL_Lower.Find(_T("://"));
	if( nPos == -1 )
	{
		csURL_Lower.Insert ( 0, _T("http://") );
		csURL.Insert ( 0, _T("http://") );
		nPos = 4;
	}
	CString csProtocolType = csURL_Lower.Left ( nPos );

	csURL_Lower = csURL_Lower.Mid( csProtocolType.GetLength()+3 );
	csURL = csURL.Mid( csProtocolType.GetLength()+3 );
	nPos = csURL_Lower.Find('/');
	if ( nPos == -1 )
		return FALSE;

	m_object = csURL.Mid(nPos);
	url_Encode(m_object);

	CString csServerAndPort = csURL_Lower.Left(nPos);

	// 查找是否有端口号，站点服务器域名一般用小写
	nPos = csServerAndPort.Find(_T(":"));
	if( nPos == -1 )
	{
		m_host	= csServerAndPort;
		m_port	= DefaultPort;
	}
	else
	{
		m_host = csServerAndPort.Left( nPos );
		CString str = csServerAndPort.Mid( nPos+1 );
		m_port	  = (USHORT)_ttoi( str.GetBuffer(str.GetLength()) );
		str.ReleaseBuffer();
	}

	if(csProtocolType != _T("http") || m_host == _T(""))
	{
		return FALSE;
	}

	return TRUE;
}

CString ChttpRequest::GetRequestStr(const CString& type, const wchar_t* post_data)
{
	CString strVerb;
	if( type == _T("get") )
	{
		strVerb = _T("GET ");
	}
	else if(type == _T("post"))
	{
		strVerb = _T("POST ");
	}
	else
	{
		ATLASSERT(0);
	}

	CString csReq, strAuth, strRange;
	csReq  = strVerb  + m_object.c_str() + _T(" HTTP/1.1\r\n");

	csReq = csReq + _T("Accept: */*\r\n");
	if( m_Referer.empty() )
	{
		m_Referer = GetRefererFromURL ();
	}
	csReq = csReq + _T("Referer: ")+m_Referer.c_str()+_T("\r\n");
	csReq = csReq + _T("User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)\r\n");

	CString csPort;
	if ( m_port != DefaultPort )
		csPort.Format ( _T(":%d"), m_port );
	csReq = csReq + _T("Host: ") + m_host.c_str() + csPort + _T("\r\n");


	//	csReq = csReq + _T("Pragma: no-cache\r\n"); 
	//	csReq = csReq + _T("Cache-Control: no-cache\r\n");


	//csReq = csReq + _T("Accept-Encoding: gzip,deflate") + _T("\r\n");
	//csReq = csReq + _T("Content-Length: 0") + _T("\r\n");
	
	if(type == _T("post"))
	{
		csReq = csReq + _T("Content-Type: text/xml\r\n");
		int post_data_size = lstrlen(post_data);
		wchar_t buf[128];
		memset(buf, 0, _countof(buf));
		_itow_s(post_data_size, buf, 128, 10);
		csReq = csReq + _T("Content-Length: ") + buf + _T("\r\n"); 
	}

	csReq = csReq + _T("Connection: Close\r\n");

	csReq = csReq + _T("\r\n");

	if(type == _T("post") && post_data != NULL)
	{
		csReq = csReq + post_data;
	}
	return csReq;
}

CString ChttpRequest::FindAfterFlagString(LPCTSTR lpszFoundStr, CString csOrg)
{
	_ASSERT ( lpszFoundStr && wcslen(lpszFoundStr) > 0 );
	CString csReturing, csFoundStr = GET_SAFE_STRING(lpszFoundStr);
	csFoundStr.MakeLower ();

	wchar_t *pTmp = new wchar_t[csOrg.GetLength() + 1];
	wcscpy_s(pTmp, csOrg.GetLength()+1, csOrg.GetBuffer(csOrg.GetLength()));
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

//BOOL ChttpRequest::SendRequest(const CString& request, CString& csResponse)
//{
//	CString temp_request = request;
//	if ( !m_SocketClient.SendString ( temp_request.GetBuffer(temp_request.GetLength()) ) )
//	{
//		return FALSE;
//	}
//	
//	for ( int i=0; ; i++ )
//	{
//		char szRecvBuf[NET_BUFFER_SIZE] = {0};
//		int nReadSize = m_SocketClient.Receive ( szRecvBuf, sizeof(szRecvBuf) );
//		if ( nReadSize <= 0 )
//		{
//			break;
//		}
//		csResponse += szRecvBuf;
//
//		int pos = csResponse.Find(_T("\r\n\r\n"));
//		if(pos > 0)
//		{
//			// 获取 Content-Length
//			CString csDownFileLen = FindAfterFlagString ( _T("content-length:"), csResponse );
//			__int64 length = (__int64) _wtoi64( csDownFileLen.GetBuffer(csDownFileLen.GetLength()) );
//			csDownFileLen.ReleaseBuffer();
//			//如果收到的长度大于等于内容长度+http head的长度
//			if(length > 0 && csResponse.GetLength() >= pos + 4 + length)
//			{
//				break;
//			}
//		}
//		
//	}
//
//	int pos = csResponse.Find(_T("\r\n\r\n"));
//	if(pos > 0)
//	{
//		csResponse = csResponse.Mid(pos + 4);
//		return TRUE;
//	}
//	
//	return FALSE;
//}

BOOL ChttpRequest::GetModifyTime(const wchar_t* url, CTime& modify_time)
{
	wchar_t c_decode_url[1024] = {0};
	URL_Decode(url, c_decode_url);
	m_Url = url;
	//varify 提取host，port，object等信息
	if ( !ParseHttpUrl ( c_decode_url ) )
	{
		return FALSE;
	}

	////connect
	//if(!m_SocketClient.Connect(m_host.c_str(), m_port))
	//{
	//	return FALSE;
	//}

	return RequestModifyTime(modify_time);
}

BOOL ChttpRequest::GetMD5(const wchar_t* url, CString& md5)
{
	wchar_t c_decode_url[1024] = {0};
	URL_Decode(url, c_decode_url);
	m_Url = url;
	//varify 提取host，port，object等信息
	if ( !ParseHttpUrl ( c_decode_url ) )
	{
		return FALSE;
	}

	////connect
	//if(!m_SocketClient.Connect(m_host.c_str(), m_port))
	//{
	//	return FALSE;
	//}

	return RequestMD5(md5);
}

BOOL ChttpRequest::RequestModifyTime(CTime& modify_time)
{
	BOOL bRetryRequest = TRUE;
	while ( bRetryRequest )
	{
		CString csReq = GetRequestStr ( _T("get") );
		CString csResponse;
		if ( !SendRequest ( csReq, csResponse, NULL, NULL ) )
			return FALSE;

		if ( !ParseModifyTime( csResponse, bRetryRequest, modify_time) )
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL ChttpRequest::RequestMD5(CString& md5)
{
	BOOL bRetryRequest = TRUE;
	while ( bRetryRequest )
	{
		CString csReq = GetRequestStr ( _T("get") );
		CString csResponse;
		if ( !SendRequest ( csReq, csResponse, NULL, NULL ) )
			return FALSE;

		if ( !ParseMD5( csResponse, bRetryRequest, md5) )
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL ChttpRequest::SendRequest(LPCTSTR lpszReq, CString &csResponse, char *szTailData/*=NULL*/, int *pnTailSize/*=NULL*/ )
{
	m_SocketClient.Disconnect ();
	if(!m_SocketClient.Connect(m_host.c_str(), m_port))
	{
		return FALSE;
	}
	csResponse = _T("");

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
			break;
		}
	}

	return TRUE;
}

DWORD ChttpRequest::GetResponseCode(CString csLineText)
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

	return (DWORD)_wtoi(csCode);
}

BOOL ChttpRequest::ParseModifyTime(CString csResponseString, BOOL &bRetryRequest, CTime& modify_time)
{
	bRetryRequest = FALSE;

	// 获取返回代码
	CString csOneLine = GetOneLine ( csResponseString );
	csResponseString.ReleaseBuffer();
	DWORD dwResponseCode = GetResponseCode ( csOneLine );

	if ( dwResponseCode < 1 )
	{
		return FALSE;
	}

	int nPos = 0;
	// 请求文件被重定向
	if( dwResponseCode >= 300 && dwResponseCode < 400 )
	{
		bRetryRequest = TRUE;
		// 得到请求文件新的URL
		CString csRedirectFileName = FindAfterFlagString ( _T("location:"), csResponseString );

		{
			// 解决文件重定向后，url被转义，导致链接到新url地址时，获取服务器回应异常（Location:标签指向的新地址错误）
			csRedirectFileName.Replace(_T("%3A"), _T(":"));
			csRedirectFileName.Replace(_T("%2F"), _T("/"));
		}

		// 设置 Referer
		m_Referer = GetRefererFromURL();

		// 重定向到其他的服务器
		nPos = csRedirectFileName.Find(_T("://"));
		if ( nPos >= 0 )
		{
			// 检验要下载的URL是否有效
			return ParseHttpUrl(csRedirectFileName.GetBuffer(csRedirectFileName.GetLength()));
		}

		// 重定向到本服务器的其他地方
		csRedirectFileName.Replace ( _T("\\"), _T("/") );
		// 重定向于根目录
		if( csRedirectFileName[0] == '/' )
		{
			m_object = csRedirectFileName.GetBuffer(csRedirectFileName.GetLength());
			return TRUE;
		}

		// 定向于相对当前目录
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
			nPos = m_Url.rfind('/');
			if (nPos != -1)
				m_Url = m_Url.substr(0, nPos);
		}
		if ( csRedirectFileName.Find ( _T("./"), 0 ) == 0 )
			csRedirectFileName.Delete ( 0, 2 );
		m_Url = m_Url+_T("/")+csRedirectFileName.GetBuffer(csRedirectFileName.GetLength());

		return ParseHttpUrl( m_Url.c_str() );
	}
	// 请求被成功接收、理解和接受
	else if( dwResponseCode >= 200 && dwResponseCode < 300 )
	{
		// 获取服务器文件的最后修改时间
		CString csModifiedTime = FindAfterFlagString ( _T("last-modified:"), csResponseString );
		if ( !csModifiedTime.IsEmpty() )
		{
			modify_time = ConvertHttpTimeString(csModifiedTime);
		}

		return TRUE;
	}

	return FALSE;
};

BOOL ChttpRequest::ParseMD5(CString csResponseString, BOOL &bRetryRequest, CString& md5)
{
	bRetryRequest = FALSE;

	// 获取返回代码
	CString csOneLine = GetOneLine ( csResponseString );
	csResponseString.ReleaseBuffer();
	DWORD dwResponseCode = GetResponseCode ( csOneLine );

	if ( dwResponseCode < 1 )
	{
		return FALSE;
	}

	int nPos = 0;
	// 请求文件被重定向
	if( dwResponseCode >= 300 && dwResponseCode < 400 )
	{
		bRetryRequest = TRUE;
		// 得到请求文件新的URL
		CString csRedirectFileName = FindAfterFlagString ( _T("location:"), csResponseString );

		{
			// 解决文件重定向后，url被转义，导致链接到新url地址时，获取服务器回应异常（Location:标签指向的新地址错误）
			csRedirectFileName.Replace(_T("%3A"), _T(":"));
			csRedirectFileName.Replace(_T("%2F"), _T("/"));
		}

		// 设置 Referer
		m_Referer = GetRefererFromURL();

		// 重定向到其他的服务器
		nPos = csRedirectFileName.Find(_T("://"));
		if ( nPos >= 0 )
		{
			// 检验要下载的URL是否有效
			return ParseHttpUrl(csRedirectFileName.GetBuffer(csRedirectFileName.GetLength()));
		}

		// 重定向到本服务器的其他地方
		csRedirectFileName.Replace ( _T("\\"), _T("/") );
		// 重定向于根目录
		if( csRedirectFileName[0] == '/' )
		{
			m_object = csRedirectFileName.GetBuffer(csRedirectFileName.GetLength());
			return TRUE;
		}

		// 定向于相对当前目录
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
			nPos = m_Url.rfind('/');
			if (nPos != -1)
				m_Url = m_Url.substr(0, nPos);
		}
		if ( csRedirectFileName.Find ( _T("./"), 0 ) == 0 )
			csRedirectFileName.Delete ( 0, 2 );
		m_Url = m_Url+_T("/")+csRedirectFileName.GetBuffer(csRedirectFileName.GetLength());

		return ParseHttpUrl( m_Url.c_str() );
	}
	// 请求被成功接收、理解和接受
	else if( dwResponseCode >= 200 && dwResponseCode < 300 )
	{
		// 获取服务器文件的MD5
		CString csMD5_Encode = FindAfterFlagString ( _T("Content-MD5:"), csResponseString );
		if(!csMD5_Encode.IsEmpty())
		{
			CString csMD5_Decode;
			if(Base64Decode(csMD5_Encode.GetBuffer(csMD5_Encode.GetLength()), csMD5_Decode))
			{
				md5 = csMD5_Decode;
			}
			else
			{
				md5 = _T("");
			}
		}
		else
		{
			md5 = _T("");
		}

		return TRUE;
	}

	return FALSE;
};

int ChttpRequest::Base64Decode(LPCTSTR lpszDecoding, CString &strDecoded)
{
	//widechartomultibyte
	int astrLen     = WideCharToMultiByte(CP_ACP, 0, lpszDecoding, -1, NULL, 0, NULL, NULL);
	char* converted = new char[astrLen];
	WideCharToMultiByte(CP_ACP, 0, lpszDecoding, -1, converted, astrLen, NULL, NULL);
	std::string decStr = converted;
	delete []converted;
	converted = NULL;

	std::string dec = Base64::decode(decStr);

	std::string strRetVal = "";
	int size = dec.size();
	for(int i = 0 ; i < size; i++)
	{
		char buf[3] = {0};
		sprintf(buf, "%02X", (unsigned char)dec.at(i));
		strRetVal += buf;
	}

	int rv_size = MultiByteToWideChar(CP_UTF8, 0, strRetVal.c_str(),strRetVal.size(), NULL, 0);
	wchar_t* woutput = new wchar_t[rv_size+1];
	memset(woutput, 0, sizeof(wchar_t)*(rv_size+1));
	MultiByteToWideChar(CP_UTF8, 0, strRetVal.c_str(),strRetVal.size(), woutput, rv_size);

	strDecoded = woutput;

	delete []woutput;
	woutput = NULL;
	return strDecoded.GetLength();
}
//
// 将 HTTP 服务器表示的时间转换为 CTime 格式，如：Wed, 16 May 2007 14:29:53 GMT
//
CTime ChttpRequest::ConvertHttpTimeString(CString csTimeGMT)
{
	CString csYear, csMonth, csDay, csTime;
	CTime tReturning = -1;
	int nPos = csTimeGMT.Find ( _T(","), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );
	csTimeGMT.TrimLeft(); csTimeGMT.TrimRight ();

	// 日
	nPos = csTimeGMT.Find ( _T(" "), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csDay = csTimeGMT.Left ( nPos );
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );
	csTimeGMT.TrimLeft(); csTimeGMT.TrimRight ();

	// 月
	nPos = csTimeGMT.Find ( _T(" "), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csMonth = csTimeGMT.Left ( nPos );
	int nMonth = GetMouthByShortStr ( csMonth );
	_ASSERT ( nMonth >= 1 && nMonth <= 12 );
	csMonth.Format ( _T("%02d"), nMonth );
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );
	csTimeGMT.TrimLeft(); csTimeGMT.TrimRight ();

	// 年
	nPos = csTimeGMT.Find ( _T(" "), 0 );
	if ( nPos < 0 || nPos >= csTimeGMT.GetLength()-1 )
		return tReturning;
	csYear = csTimeGMT.Left ( nPos );
	csTimeGMT = csTimeGMT.Mid ( nPos + 1 );
	csTimeGMT.TrimLeft(); csTimeGMT.TrimRight ();

	// 时间
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

CString ChttpRequest::GetRefererFromURL()
{
	CString temp = m_Url.c_str();
	int nPos = temp.Find('?');
	if(nPos > 0)
	{
		temp = temp.Left(nPos);
	}
	nPos = temp.ReverseFind ( '/' );
	if ( nPos < 0 ) return _T("");
	return temp.Left ( nPos );
}

BOOL ChttpRequest::ParseResponse(CString csResponseString, BOOL &bRetryRequest)
{
	bRetryRequest = FALSE;

	// 获取返回代码
	CString csOneLine = GetOneLine ( csResponseString );
	csResponseString.ReleaseBuffer();
	DWORD dwResponseCode = GetResponseCode ( csOneLine );

	if ( dwResponseCode < 1 )
	{
		return FALSE;
	}

	int nPos = 0;
	// 请求文件被重定向
	if( dwResponseCode >= 300 && dwResponseCode < 400 )
	{
		bRetryRequest = TRUE;
		// 得到请求文件新的URL
		CString csRedirectFileName = FindAfterFlagString ( _T("location:"), csResponseString );

		{
			// 解决文件重定向后，url被转义，导致链接到新url地址时，获取服务器回应异常（Location:标签指向的新地址错误）
			csRedirectFileName.Replace(_T("%3A"), _T(":"));
			csRedirectFileName.Replace(_T("%2F"), _T("/"));
		}

		// 设置 Referer
		m_Referer = GetRefererFromURL();

		// 重定向到其他的服务器
		nPos = csRedirectFileName.Find(_T("://"));
		if ( nPos >= 0 )
		{
			// 检验要下载的URL是否有效
			return ParseHttpUrl(csRedirectFileName.GetBuffer(csRedirectFileName.GetLength()));
		}

		// 重定向到本服务器的其他地方
		csRedirectFileName.Replace ( _T("\\"), _T("/") );
		// 重定向于根目录
		if( csRedirectFileName[0] == '/' )
		{
			m_object = csRedirectFileName.GetBuffer(csRedirectFileName.GetLength());
			return TRUE;
		}

		// 定向于相对当前目录
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
			nPos = m_Url.rfind('/');
			if (nPos != -1)
				m_Url = m_Url.substr(0, nPos);
		}
		if ( csRedirectFileName.Find ( _T("./"), 0 ) == 0 )
			csRedirectFileName.Delete ( 0, 2 );
		m_Url = m_Url+_T("/")+csRedirectFileName.GetBuffer(csRedirectFileName.GetLength());

		return ParseHttpUrl( m_Url.c_str() );
	}
	// 请求被成功接收、理解和接受
	else if( dwResponseCode >= 200 && dwResponseCode < 300 )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL ChttpRequest::RecvData(CString& csResponseString, char *szTailData, int pnTailSize)
{
	if(szTailData != NULL && pnTailSize > 0)
	{
		szTailData[pnTailSize] = '\0';
		csResponseString = szTailData;
	}
	
	for ( int i=0; ; i++ )
	{
		char szRecvBuf[NET_BUFFER_SIZE] = {0};
		int nReadSize = m_SocketClient.Receive ( szRecvBuf, sizeof(szRecvBuf) );
		if ( nReadSize <= 0 )
		{
			break;
		}
		csResponseString += szRecvBuf;
	
		int pos = csResponseString.Find(_T("\r\n\r\n"));
		if(pos > 0)
		{
			// 获取 Content-Length
			CString csDownFileLen = FindAfterFlagString ( _T("content-length:"), csResponseString );
			__int64 length = (__int64) _wtoi64( csDownFileLen.GetBuffer(csDownFileLen.GetLength()) );
			csDownFileLen.ReleaseBuffer();
			//如果收到的长度大于等于内容长度+http head的长度
			if(length > 0 && csResponseString.GetLength() >= pos + 4 + length)
			{
				break;
			}
		}
			
	}
		
	return TRUE;
}

