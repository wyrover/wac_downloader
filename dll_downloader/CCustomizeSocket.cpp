/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:24
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\CCustomizeSocket.cpp
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	CCustomizeSocket
	file ext:	cpp
	author:		tanyc
	
	purpose:	
*********************************************************************/
#include "stdafx.h"
#include "CCustomizeSocket.h"
#include "../../Common/util.h"

//域名解析保存多IP,12.9.24
Host2MultiIpMap CCustomizeSocket::m_addr_host;
CCustomizeSocket::CCustomizeSocket(void)
: m_hEvtEndModule ( NULL )
, m_bConnected ( FALSE )
, m_nIndex ( -1 )
, m_socket ( INVALID_SOCKET )
{
	InitSocket();
	srand((unsigned int)time(NULL));
}

CCustomizeSocket::~CCustomizeSocket()
{
	WSACleanup();
}

BOOL CCustomizeSocket::InitSocket()
{
	//初始化TCP协议
	WORD wVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	BOOL ret = WSAStartup(wVersion, &wsaData); 
	if(ret != 0) 
	{ 
		//could not find a usable WinSock DLL.
		return false;
	} 

	if ( LOBYTE( wsaData.wVersion ) != LOBYTE(wVersion) || HIBYTE( wsaData.wVersion ) != HIBYTE(wVersion))
	{
		WSACleanup();
		return false; 
	}
	return true;
}

BOOL CCustomizeSocket::ReConnect()
{
	return Connect(m_szHost.GetBuffer(m_szHost.GetLength()), m_nPort, m_dwTimeout);
}

BOOL CCustomizeSocket::Connect(LPCTSTR lpszHost, USHORT nPort, DWORD dwTimeout)
{
	Disconnect();
	if(!lpszHost)
	{
		return false;
	}

	// 记录上一次连接的参数
	m_szHost.Format(_T("%s"), lpszHost);
	m_nPort = nPort;
	m_dwTimeout = dwTimeout;
	int nErrorCode = 0;

	//创建客户端套接字 
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if(m_socket == INVALID_SOCKET)
	{ 
		nErrorCode = WSAGetLastError();
		return false; 
	} 

	ULONG lhost = CCustomizeSocket::GetHost(lpszHost);
	
	CString szLog;
	struct in_addr ia;
	ia.s_addr = lhost;
	//wchar_t szIp[128];
	//memset(szIp, 0, _countof(szIp));
	char* ip = inet_ntoa(ia);

	std::wstring szIp = s2ws(std::string(ip));
	//setlocale(LC_ALL,"");
	//mbstowcs(szIp, ip, strlen(ip));
	//setlocale(LC_ALL,"C"); 
	szLog.Format(_T("%s get host: %s"), lpszHost, szIp.c_str());
	__LOG_LOG__(szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	if(lhost <= 0)
	{
		closesocket(m_socket);
		return false;
	}

// 	struct hostent* stc_sock_host;
// 	stc_sock_host = gethostbyname(lpszHost);
// 	if(NULL == stc_sock_host)
// 	{
// 		closesocket(m_socket);
// 		return false;
// 	}

	struct sockaddr_in daddr;
	memset((void *)&daddr,0,sizeof(daddr));
	daddr.sin_family = AF_INET;
	daddr.sin_port = htons(nPort);
	daddr.sin_addr.s_addr = lhost; //*((unsigned long*)stc_sock_host->h_addr); /*inet_addr(lpszHost)*/
	int ConnectCode = connect(m_socket,(struct sockaddr *)&daddr,sizeof(daddr));

	// select 模型，设置超时
	struct timeval timeout;
	fd_set r;
	FD_ZERO(&r);
	FD_SET(m_socket, &r);
	timeout.tv_sec = dwTimeout; // 连接超时
	timeout.tv_usec = 0;
	int ret = select(0, 0, &r, 0, &timeout);
	if ( ret <= 0 )
	{	 
		// 超时
		nErrorCode = WSAGetLastError();
		return false;
	}

	// 再设回阻塞模式
	unsigned long ul1 = 0 ;
	ret = ioctlsocket(m_socket, FIONBIO, (unsigned long*)&ul1);
	if(ret == SOCKET_ERROR)
	{
		// 关闭连接
		nErrorCode = WSAGetLastError();
		closesocket(m_socket);
		return false;
	}

	m_bConnected = true;
	return true;
}

void CCustomizeSocket::Disconnect()
{
	if(INVALID_SOCKET != m_socket)
	{
		closesocket(m_socket);
	}
	m_bConnected = FALSE;
}

//
// return : -----------------------------------------------------------
//		>= 0	-	收到的字节数
//		-1		-	失败
//
int CCustomizeSocket::Receive(char *szBuf, int size, BOOL bBlock/*=TRUE*/)
{
	if(m_socket == INVALID_SOCKET)
	{
		return -1;		
	}

	if ( !szBuf || size < 0 ) return -1;
	if(!bBlock)
	{
		return -1;
	}
	int nReadSize = 0;


	// 检查可读状态并设置超时
	long dwTimeout = 10;
	TIMEVAL tv01 = {dwTimeout, 0};
	FD_SET fdr = {1, m_socket}; 
	int nSelectRet = ::select(0, &fdr, NULL, NULL, &tv01);
	if(SOCKET_ERROR == nSelectRet) 
	{ 
		return -1;
	} 
	else if(0 == nSelectRet) 
	{ 
		//超时发生，无可读数据
		return -1;
	}

	nReadSize = recv(m_socket, szBuf, size, 0);
	if(nReadSize == 0)
	{
		// 连接已断开
		closesocket(m_socket);
		m_bConnected = FALSE;
		return -1;
	}
	else if(nReadSize == SOCKET_ERROR)
	{
		// 错误
		return -1;
	}

	/*
	bool bReConnectRet = false;
	int nReConnectTimes = 3;
	int nRetryTimes = 3;
	while(nRetryTimes >= 0)
	{
		if( ::WaitForSingleObject ( m_hEvtEndModule, 1 ) == WAIT_OBJECT_0 )
		{
			nReadSize = 0;
			break;
		}

		// 检查可读状态并设置超时
		long dwTimeout = 15;
		TIMEVAL tv01 = {dwTimeout, 0};
		FD_SET fdr = {1, m_socket}; 
		int nSelectRet = ::select(0, &fdr, NULL, NULL, &tv01);
		if(SOCKET_ERROR == nSelectRet) 
		{ 
			return -1;
		} 
		else if(0 == nSelectRet) 
		{ 
			//超时发生，无可读数据
			return -1;
		}

		nReadSize = recv(m_socket, szBuf, size, 0);
		if(nReadSize == 0)
		{
			// 连接已断开
			closesocket(m_socket);
			m_bConnected = FALSE;
			while(nReConnectTimes)
			{
				if( ::WaitForSingleObject ( m_hEvtEndModule, 1 ) == WAIT_OBJECT_0 )
				{
					bReConnectRet = false;
					break;
				}
				if(ReConnect())
				{
					bReConnectRet = true;
					break;
				}
				nReConnectTimes--;
			}

			if(!bReConnectRet)
			{
				// 重新连接N次后，依然连接失败
				closesocket(m_socket);
				m_bConnected = FALSE;
				return -1;
			}
		}
		else if(nReadSize == SOCKET_ERROR)
		{
			// 错误
			return -1;
		}
		else
		{
			break;
		}
		nRetryTimes--;
	}
	*/

	return nReadSize;
}

//
// return : ------------------------------------------
//		> 0		-	回应代码
//		= 0		-	未读到数据
//		= -1	-	网络出错
//
int CCustomizeSocket::GetResponse ( CString *pcsResponseStr/*=NULL*/, BOOL bBlock/*=TRUE*/ )
{
	if ( pcsResponseStr ) *pcsResponseStr = "";
	_ASSERT ( m_socket != INVALID_SOCKET );
	CString csOneLine = GetOneLine ( m_csResponseHistoryString );
	m_csResponseHistoryString.ReleaseBuffer();
	if ( csOneLine.IsEmpty () )
	{
		char szRecvBuf[NET_BUFFER_SIZE] = {0};
		int nRet = Receive ( szRecvBuf, sizeof(szRecvBuf), bBlock );
		if ( nRet <= 0 )
		{
			if ( nRet < 0 )
			{
				CString szLog;
				szLog.Format(_T("(%d) Receive response data failed."), m_nIndex);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
			}
			return nRet;
		}
		m_csResponseHistoryString += szRecvBuf;
		csOneLine = GetOneLine ( m_csResponseHistoryString );
		m_csResponseHistoryString.ReleaseBuffer();
		if ( csOneLine.IsEmpty () ) return -1;
	}

	if ( pcsResponseStr ) *pcsResponseStr = csOneLine;
	CString csRetCode = GetDigitStrAtHead ( csOneLine );

	CString szLog;
	szLog.Format(_T("Sub download thread(%d), FTP server response: %s."), m_nIndex, csOneLine);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	return _wtoi ( csRetCode );
}

BOOL CCustomizeSocket::GetResponse ( int nVerifyCode, CString *pcsResponseStr/*=NULL*/ )
{
	CString csResponseStr;
	int nResponseCode = GetResponse ( &csResponseStr );
	if ( pcsResponseStr ) *pcsResponseStr = csResponseStr;
	if ( nResponseCode != nVerifyCode )
	{
		CString csMsg;
		csMsg.Format ( _T("Receive error response code : %s"), csResponseStr );
		return FALSE;
	}
	return TRUE;
}

BOOL CCustomizeSocket::SendString(LPCTSTR lpszData, ...)
{
	// 格式化
	wchar_t szDataBuf[NET_BUFFER_SIZE] = {0};
	va_list  va;
	va_start (va, lpszData);
	_vsnwprintf ( szDataBuf, sizeof(szDataBuf)-1, (const wchar_t*)lpszData, va);
	va_end(va);

	//CString szLog;
	//szLog.Format(_T("Sub download thread(%d), Send string : \r\n------------------------>>>\r\n%s<<<------------------------"), m_nIndex, szDataBuf);
	//__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	int astrLen     = WideCharToMultiByte(CP_ACP, 0, szDataBuf, -1, NULL, 0, NULL, NULL);
	char* converted = new char[astrLen];
	WideCharToMultiByte(CP_ACP, 0, szDataBuf, -1, converted, astrLen, NULL, NULL);
	BOOL rv = Send ( converted, astrLen );
	delete []converted;
	return rv;
}

BOOL CCustomizeSocket::SendString2(LPCTSTR lpszData)
{
	// 格式化
	wchar_t szDataBuf[NET_BUFFER_SIZE] = {0};
	wcscpy(szDataBuf, lpszData);

	CString szLog;
	szLog.Format(_T("Sub download thread(%d), Send string : \r\n------------------------>>>\r\n%s<<<------------------------"), m_nIndex, szDataBuf);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	int astrLen     = WideCharToMultiByte(CP_ACP, 0, szDataBuf, -1, NULL, 0, NULL, NULL);
	char* converted = new char[astrLen+1];
	memset(converted, 0, astrLen+1);
	WideCharToMultiByte(CP_ACP, 0, szDataBuf, -1, converted, astrLen, NULL, NULL);
	BOOL rv = Send ( converted, astrLen );
	delete []converted;
	return rv;
}

BOOL CCustomizeSocket::Send(char *data, int size)
{
	//_ASSERT ( m_hEvtEndModule && m_hEvtEndModule != INVALID_HANDLE_VALUE );
	if(m_socket == INVALID_SOCKET)
	{
		return FALSE;		
	}
	if ( !data || size < 1 ) return TRUE;

	// 检查可写状态，并设置超时
	long dwTimeout = 15;
	TIMEVAL tv01 = {dwTimeout, 0};
	FD_SET fdw = {1, m_socket}; 
	int nSelectRet = ::select(0, NULL, &fdw,NULL, &tv01);
	if(SOCKET_ERROR == nSelectRet) 
	{ 
		return FALSE;
	} 
	else if(0 == nSelectRet) 
	{ 
		//超时发生，缓冲满或网络忙
		return FALSE;
	} 
	else 
	{ 
		//发送
		int ret = send(m_socket, data, size, 0);
		if(ret == SOCKET_ERROR)
		{
			return FALSE;
		}
		return TRUE;
	}
}

void CCustomizeSocket::SetEventOfEndModule(HANDLE hEvtEndModule)
{
	m_hEvtEndModule = hEvtEndModule;
	//_ASSERT ( m_hEvtEndModule && m_hEvtEndModule != INVALID_HANDLE_VALUE );
}


CString CCustomizeSocket::GetDigitStrAtHead ( LPCTSTR lpszStr )
{
	if ( !lpszStr ) return _T("");
	CString csStr = lpszStr;
	csStr.TrimLeft(); csStr.TrimRight();
	CString csDigitStr;
	for ( int i=0; isdigit ( (int)csStr[i] ); i++ )
	{
		csDigitStr += csStr[i];
	}

	return csDigitStr;
}

// 从类似 "(192,168,0,2,4,31)" 字符串中得到IP地址和端口号
//
BOOL CCustomizeSocket::GetIPAndPortByPasvString(LPCTSTR lpszPasvString, OUT CString &csIP, OUT USHORT &nPort)
{
	if ( !lpszPasvString ) return FALSE;
	const wchar_t *p = wcsstr( lpszPasvString, _T("(") );
	if ( !p ) return FALSE;
	CString csPasvStr = p+1, csTemp;
	int nPosStart = 0, nPosEnd = 0;
	int nMultiple = 0, nMantissa = 0;
	for ( int i=0; ; i++ )
	{
		nPosEnd = csPasvStr.Find ( _T(","), nPosStart );
		if ( nPosEnd < 0 )
		{
			if ( i == 5 )
			{
				nPosEnd = csPasvStr.Find ( _T(")"), nPosStart );
				csTemp = csPasvStr.Mid ( nPosStart, nPosEnd-nPosStart );
				nMantissa = _wtoi ( csTemp );
				break;
			}
			else return FALSE;
		}
		csTemp = csPasvStr.Mid ( nPosStart, nPosEnd-nPosStart );
		csTemp.TrimLeft(); csTemp.TrimRight();
		if ( i < 4 )
		{
			if ( !csIP.IsEmpty () ) csIP += _T(".");
			csIP += csTemp;
		}
		else if ( i == 4 )
		{
			nMultiple = _wtoi ( csTemp );
		}
		else return FALSE;
		nPosStart = nPosEnd + 1;
	}
	nPort = nMultiple*256 + nMantissa;

	return TRUE;
}

ULONG CCustomizeSocket::GetHost(LPCTSTR lpszHost)
{
	static CThreadLock	locker;
	MyAutolock<CThreadLock> _lk(locker);


	Host2MultiIpMap::const_iterator iter;
	iter = m_addr_host.find(lpszHost);
	if(iter != m_addr_host.end())

	{
		//随机返回一个IP,2012.9.24
		//srand((unsigned int)time(NULL));
		int size = iter->second.size();
		int seq = rand() % size;
		return iter->second[seq];
	}
	else
	{
		ULONG u_addr = 0;
		struct hostent* stc_sock_host = NULL;
		USES_CONVERSION;
		stc_sock_host = gethostbyname(W2A(lpszHost));
		if(NULL == stc_sock_host)
		{
			u_addr = 0;
		}
		else
		{
			//u_addr = *((unsigned long*)stc_sock_host->h_addr);
			//typedef std::pair<std::wstring, ULONG>Host_Pair;
			//m_addr_host.insert(Host_Pair(lpszHost, u_addr));
			std::vector<ULONG> ip_vec;
			for(char** pptr = stc_sock_host->h_addr_list; *pptr != NULL; ++pptr)
			{
				ip_vec.push_back(*((unsigned long*)(*pptr)));
			}

			//srand((unsigned int)time(NULL));
			int seq = rand() % ip_vec.size();
			u_addr = ip_vec[seq];
			m_addr_host.insert(std::make_pair(lpszHost, ip_vec));


			CString szLog;
			szLog.Format(_T("%s get host count: %d"), lpszHost, ip_vec.size());
			__LOG_LOG__(szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		}
		return u_addr;
	}
}
void CCustomizeSocket::ClearHost()
{
	m_addr_host.clear();
}