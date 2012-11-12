#pragma once
#include <string>
#include "CCustomizeSocket.h"

class ChttpRequest
{
public:
	ChttpRequest(void);
	~ChttpRequest(void);

	int Get(const std::wstring& url, wchar_t* content, int& size);
	int Post(const std::wstring& url, const wchar_t* post_data, wchar_t* content, int& size);

	BOOL GetModifyTime(const wchar_t* url, CTime& modify_time);

	BOOL GetMD5(const wchar_t* url, CString& md5);

	static const USHORT DefaultPort = 80;
private:
	BOOL ParseHttpUrl(LPCTSTR lpszURL);
	CString GetRequestStr(const CString& type, const wchar_t* post_data = NULL);
	//BOOL SendRequest(const CString& request, CString& response);
	CString FindAfterFlagString(LPCTSTR lpszFoundStr, CString csOrg);
	BOOL RequestModifyTime(CTime& modify_time);
	BOOL SendRequest(LPCTSTR lpszReq, CString &csResponse, char *szTailData=NULL, int *pnTailSize=NULL );
	CTime ConvertHttpTimeString(CString csTimeGMT);
	BOOL ParseModifyTime(CString csResponseString, BOOL &bRetryRequest, CTime& modify_time);
	CString GetRefererFromURL();
	DWORD GetResponseCode(CString csLineText);
	BOOL RecvData(CString& csResponseString, char *szTailData, int pnTailSize );
	BOOL ParseResponse(CString csResponseString, BOOL &bRetryRequest);
	void url_Encode(std::wstring& str);
	BOOL RequestMD5(CString& md5);
	BOOL ParseMD5(CString csResponseString, BOOL &bRetryRequest, CString& md5);
	int Base64Decode(LPCTSTR lpszDecoding, CString &strDecoded);


private:
	CCustomizeSocket m_SocketClient;
	std::wstring m_host;
	std::wstring m_object;
	std::wstring m_Referer;
	std::wstring m_Url;
	USHORT m_port;
};

