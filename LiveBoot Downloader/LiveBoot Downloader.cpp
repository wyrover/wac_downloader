/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:22
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader\LiveBoot Downloader.cpp
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader
	file base:	LiveBoot Downloader
	file ext:	cpp
	author:		tanyc
	
	purpose:	
*********************************************************************/

#include "stdafx.h"
#include "LiveBoot Downloader.h"
#include <atlstr.h>
#include <comdef.h>
#include <shellapi.h>

#include <iostream>

#include "../dll_downloader/DownloaderManagerFactory.h"
#include "../dll_downloader/ChttpRequest.h"
#include "../../Common/util.h"
#include "../../Common/IPCSender.h"
#include "../../Common/HttpRequest.h"
#include "../../Common/server_interface_url.h"
//for iwebbrowser2
CComModule _Module;

// link common control version 6.0
// progress required
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "\
	"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		break;
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	case WM_CLOSE:
		{
			if(g_IsDownloading)
			{
				if(IDNO == ::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_QUIT_while_downloading), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
				{
					return 0;
				}
			}
			else
			{
				if(IDNO == ::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_QUIT_while_not_downloading), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
				{
					return 0;
				}
			}
			break;
		}
	//case WM_TRAY_NOTIFYCATION:
	//	{
	//		OnTrayProc(hWnd, wParam, lParam);
	//		break;
	//	}
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

//BOOL WritePicFile(const std::wstring& picPath, const char* buffer, size_t size)
//{
//	FILE* fp = _tfopen(picPath.c_str(), _T("wb"));
//	if (!fp)
//	{
//		std::wcout << "open file failed: " << picPath << std::endl;
//		return FALSE;
//	}
//	fwrite(buffer, size, 1, fp);
//	fclose(fp);
//	return TRUE;
//}

std::wstring make_post_data()
{
	//post数据示例
	/*<download_track client_sign=”{32C09B91-8B93-41CB-83FF-3E5B100EB47F}” product_id =”6”>
		<download_start>2012-09-03 12:00:00</download_start>
		<download_end>2012-09-03 12:30:00</download_end>
		<download_status>ok</download_status>
		<install_start>2012-09-03 12:30:00</install_start>
		<install_end>2012-09-03 12:30:00</install_end>
		<install_status>ok</install_status>
	</ download_track>*/
	//CString szLog;
	//szLog.Format(_T("GetClientSign: %s"), GetClientSign().c_str());
	//__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	std::wstring client_sign = GetClientSign(true);
	CMarkup post_xml;
	post_xml.SetDoc(_T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"));
	post_xml.AddElem(_T("download_track"));

	post_xml.AddAttrib(_T("client_sign"), client_sign.c_str());
	wchar_t pid[16];
	memset(pid, 0, _countof(pid));
	_itow(g_ProductPID, pid, 10);
	post_xml.AddAttrib(_T("product_id"), pid);
		
	post_xml.IntoElem();
	post_xml.AddElem(_T("download_start"), g_task_track.download_start.c_str());
	post_xml.AddElem(_T("download_end"), g_task_track.download_end.c_str());
	post_xml.AddElem(_T("download_status"), g_task_track.download_status.c_str());
	post_xml.AddElem(_T("install_start"), g_task_track.install_start.c_str());
	post_xml.AddElem(_T("install_end"), g_task_track.install_end.c_str());
	post_xml.AddElem(_T("install_status"), g_task_track.install_status.c_str());
	//增加当前时间
	post_xml.AddElem(_T("request_time"), CurrentFormatTime());
	post_xml.OutOfElem();

	return post_xml.GetDoc();

	//std::wstring post_data = post_xml.GetDoc();
	//char post_buffer[MAX_RESP_BUF];
	//memset(post_buffer, 0, sizeof(post_buffer));
	//int n = wcstombs(post_buffer,post_data.c_str(),post_data.size());

	////DEBUG
	//std::cout << "--------- post data ----------" << std::endl;
	//std::cout << "post data: " << post_buffer << std::endl;
	//std::cout << "--------- end post data ----------" << std::endl;
	//WritePicFile(_T("C:\\Users\\Administrator\\AppData\\Roaming\\Wondershare\\post_data.xml"), post_buffer, n);
	//return std::string(post_buffer);
}
////上报下载过程跟踪信息到服务器
//BOOL ReportTrackInfoToServer()
//{
//	//请求服务器，post
//	char url[MAX_URL_BUF];
//	_snprintf(url, sizeof(url), "%s", DWN_TRACK_URL);
//
//	std::string post_data = make_post_data();
//
//	char  content[MAX_RESP_BUF+1]={0};
//	size_t size = 0;
//	int ret = http_post(url, post_data.c_str(), content, size);
//	if(ret != 0)
//	{
//		//http get 请求失败
//		std::cout << "http_post failed: " << url << std::endl;
//		return FALSE;
//	}
//
//	return TRUE;
//}

//上报下载过程跟踪信息到服务器
BOOL ReportTrackInfoToServer()
{
	if(g_already_report == TRUE)
	{
		return FALSE;
	}
	CString szReportLog;
	szReportLog.Format(_T("ReportTrackInfoToServer"));
	__LOG_LOG__( szReportLog.GetBuffer(szReportLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	//请求服务器，post
	//std::wstring url = _T(DWN_TRACK_URL);
	std::string server_url = ServerInterfaceUrl::GetInstance()->DownTrackUrl;

	std::wstring url = s2ws(server_url);

	url += _T("?client_sign=");
	url += GetClientSign(true);

	std::wstring post_data = make_post_data();

	wchar_t content[MAX_RESP_BUF];
	memset(content, 0, _countof(content));
	int size = _countof(content);

	ChttpRequest hRequest;
	if(hRequest.Post(url, post_data.c_str(), content, size) != 0)
	{
		//http get 请求失败
		std::wcout << _T("http_post failed: ") << url << std::endl;
		CString szLog;
		szLog.Format(_T("http_post failed, url: %s"), url.c_str());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_DEBUG);
		return FALSE;
	}

	CString szLog;
	szLog.Format(_T("http_post succ, url: %s\npost_data: %s\ncontent: %s"), url.c_str(), post_data.c_str(), content);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	
	g_already_report = TRUE;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//获取格式化的时间 like 2012-09-03 12:30:00
std::wstring CurrentFormatTime()
{
	time_t ltime;
	struct tm today;
	time( &ltime );
	_localtime64_s( &today, &ltime );
	wchar_t tmpbuf[128];
	memset(tmpbuf, 0, _countof(tmpbuf));
	wcsftime(tmpbuf, _countof(tmpbuf), _T("%Y-%m-%d %H:%M:%S"), &today);
	return tmpbuf;
}

int GetPidFromUrl(const CString& url)
{
	int pos = url.Find(_T("_full"));
	int length = url.GetLength();
	CString pid;
	if(pos > 0)
	{
		pos  = pos + 5;
		while(pos < length)
		{
			if('0' <= url[pos] && url[pos] <= '9')
			{
				pid += url[pos++];
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		pos = url.Find(_T("_trial"));
		if(pos > 0)
		{
			pos  = pos + 6;
			while(pos < length)
			{
				if('0' <= url[pos] && url[pos] <= '9')
				{
					pid += url[pos++];
				}
				else
				{
					break;
				}
			}
		}
	}

	if(pid.GetLength() > 0)
	{
		return _wtoi(pid.GetBuffer(pid.GetLength()));
	}
	else
	{
		return -1;
	}
}

void GetProductDownUrlFromExe()
{
	//read from exe, 偏移位置为128
	int posistion = 128;
	//宽字符版
	wchar_t UrlBuf[512];
	memset(UrlBuf, 0, _countof(UrlBuf));
	//本程序全路径
	wchar_t ExeBuf[1024];
	memset(ExeBuf, 0, _countof(ExeBuf));
	//本程序全路径
	char mbExeBuf[1024];
	memset(mbExeBuf, 0, sizeof(mbExeBuf));

	//存放读取处理的字符
	char mbUrlBuf[512];
	memset(mbUrlBuf, 0, sizeof(mbUrlBuf));

	//获取全路径
	GetModuleFileName(NULL, ExeBuf, _countof(ExeBuf));
	w2a(ExeBuf, mbExeBuf, sizeof(mbExeBuf));
	//wcstombs(mbExeBuf, ExeBuf, lstrlen(ExeBuf));
	FILE* fp = fopen(mbExeBuf, "rb");
	if(fp == NULL)
	{
		return;
	}
	if(fseek(fp, posistion, SEEK_SET) == 0)
	{
		if(fread(mbUrlBuf, sizeof(mbUrlBuf), 1, fp) == 1)
		{
			//成功
			a2w(mbUrlBuf, UrlBuf, _countof(UrlBuf));
			//mbstowcs(UrlBuf, mbUrlBuf, strlen(mbUrlBuf));
			g_DownloadURL = UrlBuf;
		}
		else
		{
			return;
		}
	}
	fclose(fp);

	//g_DownloadURL = _T("http://download.anypdftools.com/anybizsoft-pdf-converter_full524.exe");

	//没有找到正则库，直接获取PID
	//规则：preg_match("/(.+?_)(trial|full)([0-9]*)/", $download_url, $rs);
	g_ProductPID = GetPidFromUrl(g_DownloadURL);
	
}

std::wstring RequestMD5(const std::wstring& md5_url)
{
	std::wstring ret_md5;
	//char url[MAX_URL_LENGTH];
	//memset(url, 0, MAX_URL_LENGTH);

	//wcstombs(url, md5_url.c_str(), md5_url.size());

	wchar_t content[MAX_RESP_BUF];
	memset(content, 0, _countof(content));
	int ret_size = _countof(content);
	ChttpRequest hRequest;
	if(hRequest.Get(md5_url, content, ret_size) != 0)
	//if(http_get(url, content, ret_size) != 0)
	{
		CString szLog;
		szLog.Format(_T("http_get failed, url: %s"), md5_url.c_str());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		return _T("");
	}
	
	CString szLog;
	szLog.Format(_T("http_get succ, url: %s\ncontent: %s"), md5_url.c_str(), content);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	//wchar_t wcontent[MAX_RESP_BUF];
	//memset(wcontent, 0, MAX_RESP_BUF);
	//mbstowcs(wcontent, content, ret_size);

	MCD_STR str(content);

	CMarkup xml(str);

	if(xml.FindElem(_T("wsrp")))
	{
		//xml.IntoElem();
		std::wstring status = xml.GetAttrib(_T("status"));
		if(status != _T("ok"))
		{
			return _T("");
		}
		if(xml.FindChildElem(_T("parts")))
		{
			xml.IntoElem();
			ret_md5 = xml.GetAttrib(_T("MD5"));
			xml.OutOfElem();
		}
		//xml.OutOfElem();
	}
	return ret_md5;
}

//////////////////////////////////////////////////////////////////////////
//请求pid信息，包括urls，产品名称，md5值
void RequestPIDInfo()
{
	g_pid_info.ad_page_urls_.clear();
	g_pid_info.finish_page_urls_.clear();

	//std::string url = "http://10.10.24.83/down_info_";
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
	//wcstombs(downurl_buf, g_DownloadURL.GetBuffer(g_DownloadURL.GetLength()), g_DownloadURL.GetLength());
	//url += downurl_buf;

	//char pidBuf[10];
	//memset(pidBuf, 0, sizeof(pidBuf));
	//itoa(g_ProductPID, pidBuf, 10);
	//url += pidBuf;
	//url += ".xml";

	//std::wstring url = _T(DWN_INFO_URL);

	//CString szLog;
	//szLog.Format(_T("GetClientSign: %s"), GetClientSign().c_str());
	//__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	std::string server_url = ServerInterfaceUrl::GetInstance()->DownInfoUrl;
	std::wstring url = s2ws(server_url);
	url += _T("?client_sign=");
	url += GetClientSign(true);
	url += _T("&download_url=");
	url += g_DownloadURL.GetBuffer(g_DownloadURL.GetLength());

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
		return;
	}

	CString szLog;
	szLog.Format(_T("http_get succ, url: %s\ncontent: %s"), url.c_str(), content);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	//wchar_t wcontent[MAX_RESP_BUF];
	//memset(wcontent, 0, MAX_RESP_BUF);
	//mbstowcs(wcontent, content, ret_size);

	MCD_STR str(content);
	std::wstring md5_link;

	CMarkup xml(str);

	if(xml.FindElem(_T("wsrp")))
	{
		if(xml.GetAttrib(_T("status")) != _T("ok"))
		{
			return;
		}
		if(xml.FindChildElem(_T("urls")))
		{
			xml.IntoElem();
			while (xml.FindChildElem(_T("link")))
			{
				xml.IntoElem();
				std::wstring name = xml.GetAttrib(_T("name"));
				if(name == _T("ad_page"))
				{
					g_pid_info.ad_page_urls_.push_back(xml.GetAttrib(_T("href")));
				}
				else if(name == _T("finish_page"))
				{
					g_pid_info.finish_page_urls_.push_back(xml.GetAttrib(_T("href")));
				}
				
				xml.OutOfElem();
			}
			xml.OutOfElem();
		}

		xml.ResetChildPos();

		if(xml.FindChildElem(_T("md5_link")))
		{
			xml.IntoElem();
			md5_link = xml.GetAttrib(_T("href"));
			std::wstring strMD5 = RequestMD5(md5_link);
			g_MD5.Format(_T("%s"), strMD5.c_str());
			xml.OutOfElem();
		}

		xml.ResetChildPos();

		if(xml.FindChildElem(_T("product")))
		{
			xml.IntoElem();
			g_pid_info.product_name_ = xml.GetAttrib(_T("name"));
			xml.OutOfElem();
		}

		xml.ResetChildPos();

		if(xml.FindChildElem(_T("box_link")))
		{
			xml.IntoElem();
			std::wstring href = xml.GetAttrib(_T("href"));
			g_wac_url.Format(_T("%s"), href.c_str());
			g_WacPID = GetPidFromUrl(g_wac_url);
			xml.OutOfElem();
		}
	}

}
/*
 *向主程序发送程序安装的消息
 */
bool SendInstallMessage(int  pid,int rtnCode)
{
						//Send Message to Box --add by wfu---
						//主程序路径固定
						//CString sz_MainExePath =GetMainExePath(); 
						//::MessageBox(NULL, sz_MainExePath, _T("ERROR"), MB_ICONERROR);
						//RunExe(sz_MainExePath, NULL,true,false);
						CIPCSender<T_Message_Install> sender;
						sender.init(PIPE_NAME_DWN);
						T_Message_Install installMsg;
						installMsg.install_return = rtnCode;
						installMsg.pid = pid   ; //

						if(!sender.SendData(installMsg)){
							std::cout<<"Send Message failed"<<std::endl;
							
						}else{
							std::cout<<"Send Message success"<<std::endl;
						}
						//Send Message to Box --add by wfu---
						return true;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPTSTR    lpCmdLine,
					 int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	g_hInst = hInstance;

	//////////////////////////////////////////////////////////////////////////
	//get DownUrl from exe's some position
	GetProductDownUrlFromExe();
	if(g_ProductPID == -1)
	{
		::MessageBox(NULL, _T("GetProductDownUrlFromExe failed"), _T("ERROR"), MB_ICONERROR);
		return FALSE;
	}

	// log
	if(!InitLog(GetExeRunDirectory(g_hInst) + LOG_UI_FILE_NAME))
	{
		__LOG_LOG__( _T("log init failed."), L_LOGSECTION_APP_GENERAL, L_LOGLEVEL_ERROR);
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////////
	//load xml file from server,可以得到产品名称，广告页面url,box下载url

	//DWORD start = GetTickCount();
	RequestPIDInfo();
	//DWORD end = GetTickCount();
	//wchar_t buf[128] = {0};
	//wsprintf(buf, _T("RequestPIDInfo use time: %d"), end - start);
	//::MessageBox(NULL, buf, _T("ERROR"), MB_ICONERROR);

	if(g_pid_info.product_name_ == _T(""))
	{
		g_pid_info.product_name_ = DEFAULT_WINDOWS_TITLE;
	}

	//////////////////////////////////////////////////////////////////////////
	g_downtask_xml_filename.Format(_T("DownTask_%d.xml"), g_ProductPID);

	// sigle instance & active last instance
	{
		//将标识符加上后缀，防止不同的两个产品同时下载，第二个起不来
		CString CheckName;
		CheckName.Format(_T("%s_%d"), SINGLE_INSTANCE_CHECK_NAME, g_ProductPID);
		HANDLE hSem = ::CreateSemaphore(NULL, 1, 1, CheckName);
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			::CloseHandle(hSem);

			HWND hWndPrevious = ::FindWindow(g_szWindowClass, StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title));
			if(hWndPrevious)
			{
				if (IsIconic(hWndPrevious))
				{
					ShowWindow(hWndPrevious, SW_RESTORE);
				}
				else
				{
					ShowWindow(hWndPrevious, SW_SHOW);
				}
				::SetForegroundWindow(::GetLastActivePopup(hWndPrevious));
			}
			return FALSE;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//check if need download box
	HANDLE hSem = ::CreateSemaphore(NULL, 1, 1, SINGLE_INSTANCE_BOX_CHECK_NAME);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		g_need_download_box = FALSE;
	}
	if(g_wac_url == _T(""))
	{
		g_need_download_box = FALSE;
	}

	if(!GetDownloadConfigInfoFromResource(hInstance))
	{
		return FALSE;
	}

	// read config file for uncompleted task
	BOOL bNewDownload = TRUE;
	CString sz_LastTaskFilePath;
	CString sz_LastTaskFileName;
	CString sz_BoxTaskFileName;
	BOOL b_LastTaskComplete = FALSE;
	if(ReadConfig(MyGetDefaultFolder() + g_downtask_xml_filename, sz_LastTaskFilePath, sz_LastTaskFileName, sz_BoxTaskFileName, b_LastTaskComplete))
	{
		// 查看文件是否存在
		CString sz_LastTaskFilePathName = sz_LastTaskFilePath + sz_LastTaskFileName;
		if(!IsFileExist(sz_LastTaskFilePathName))
		{
			// not exist
			bNewDownload = TRUE;
			::DeleteFile(MyGetDefaultFolder() + g_downtask_xml_filename);
		}
		else
		{
			// exist,检查md5是否一致，不一致重新下载，一致转到Install now界面
			if(b_LastTaskComplete)
			{
				// md5 check
				//box需要下载，且上次没有下载box
				if(g_need_download_box && sz_BoxTaskFileName == _T(""))
				{
					CString szLog;
					szLog.Format(_T("box not download last time, need to download this time."));
					__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

					bNewDownload = TRUE;
					::DeleteFile(MyGetDefaultFolder() + g_downtask_xml_filename);
					goto START_DOWNLOAD;
				}
				//box需要下载，且已经下载过，检查md5值
				if(g_need_download_box && sz_BoxTaskFileName != _T(""))
				{
					CString sz_BoxPath = sz_LastTaskFilePath + sz_BoxTaskFileName;
					if(!IsFileExist(sz_BoxPath))
					{
						CString szLog;
						szLog.Format(_T("box file not exist, path: %s"), sz_BoxPath);
						__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

						bNewDownload = TRUE;
						::DeleteFile(MyGetDefaultFolder() + g_downtask_xml_filename);
						goto START_DOWNLOAD;
					}

					CString szBoxMd5 =  GetFileMD5(sz_BoxPath.GetBuffer(sz_BoxPath.GetLength()));
					CString szServerMd5 = GetServerMd5(g_wac_url);
					if(0 != szBoxMd5.CompareNoCase(szServerMd5))
					{
						CString szLog;
						szLog.Format(_T("box check md5 failed, url: %s"), g_wac_url);
						__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

						bNewDownload = TRUE;
						::DeleteFile(MyGetDefaultFolder() + g_downtask_xml_filename);
						goto START_DOWNLOAD;
					}

					g_wac_RealSaveFileName = sz_BoxTaskFileName;
				}

				//检查产品MD5
				CString szMd5 = GetFileMD5(sz_LastTaskFilePathName.GetBuffer(sz_LastTaskFilePathName.GetLength()));
				CString szServerMd5 = GetServerMd5(g_DownloadURL);

				if(0 == szMd5.CompareNoCase(szServerMd5))
				{
					//设置为非全新安装，进入下载对话框，进而自动进入install now界面
					g_Real_SaveFileName = sz_LastTaskFileName;
					g_wac_DownSucc = TRUE;
					g_ProductDownSucc = TRUE;
					bNewDownload = FALSE;
					g_into_install = TRUE;

					//初始化g_task_track，开始下载
					g_task_track.download_start = CurrentFormatTime();
					g_task_track.download_status = _T("unknown");
					g_task_track.install_status = _T("unknown");

					////进入安装对话框
					//if(IDYES == ::MessageBox(NULL, StringFromIDResource(g_hInst, ID_MSG_lang_LastDownload_LaunchApp), 
					//					StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
					//{
					//	// launch app
					//	g_task_track.install_start = CurrentFormatTime();
					//	CString sz_boxFilePathName = sz_LastTaskFilePath + _T("WondershareApplicationCenter_full1292.exe");
					//	RunExe(sz_boxFilePathName, NULL,true,true);
					//	int rtnCode = RunExe(sz_LastTaskFilePathName, NULL,false,true);
					//	SendInstallMessage(g_ProductPID,rtnCode);
					//	g_task_track.install_end = CurrentFormatTime();
					//	if(rtnCode == 0)
					//	{
					//		g_task_track.install_status = _T("ok");
					//	}
					//	else
					//	{
					//		g_task_track.install_status = _T("fail");
					//	}
					//	ReportTrackInfoToServer();
					//}
					//return TRUE;
				}
				else
				{
					// wrong md5，need redownload
					CString szLog;
					szLog.Format(_T("product， wrong md5 File(%s) - MD5(%s) need: %s"), sz_LastTaskFilePathName, szMd5, szServerMd5);
					__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

					::DeleteFile(sz_LastTaskFilePathName);
					::DeleteFile(MyGetDefaultFolder() + g_downtask_xml_filename);
					bNewDownload = TRUE;
				}
			}
			else
			{
				g_IsNeverDownloaded = FALSE;
				bNewDownload = FALSE;

				//if(IDYES == ::MessageBox(NULL, StringFromIDResource(g_hInst, ID_MSG_lang_LastDownload_Detect),
				//				StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
				//{
				//	// continue
				//	bNewDownload = FALSE;
				//}
				//else
				//{
				//	// exit
				//	return TRUE;
				//}
			}
		}
	}

START_DOWNLOAD:

	MSG msg;
	MyRegisterClass(hInstance);
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	// gdi+ init
	if(Ok != GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL))
	{
		return FALSE;
	}

	if(!Init(hInstance))
	{
		Clean();
		GdiplusShutdown(gdiplusToken);
		::MessageBox(NULL, StringFromIDResource(g_hInst, ID_MSG_lang_App_Init_Failed),
							StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_OK | MB_ICONINFORMATION);
		return FALSE;
	}

	// create dialog
	HWND hWndDialog = NULL;
	g_DownLoadSaveFolder = MyGetDefaultFolder();

	hWndDialog = ::CreateDialog(hInstance, (LPCTSTR)IDD_DIALOG_DOWNLOAD, g_hMainWnd, DownLoadDlgProc);

	//if(bNewDownload)
	//{
	//	g_DownLoadSaveFolder = MyGetDefaultFolder();
	//	hWndDialog = ::CreateDialog(hInstance, (LPCTSTR)IDD_DIALOG_MAIN, g_hMainWnd, MainDlgProc);
	//}
	//else
	//{
	//	g_DownLoadSaveFolder = sz_LastTaskFilePath;
	//	hWndDialog = ::CreateDialog(hInstance, (LPCTSTR)IDD_DIALOG_DOWNLOAD, g_hMainWnd, DownLoadDlgProc);
	//}
	if(NULL == hWndDialog)
	{
		Clean();
		GdiplusShutdown(gdiplusToken);
		return FALSE;
	}
	ShowWindow(hWndDialog, SW_SHOWNORMAL);

	__LOG_LOG__( _T("App init suceessful."), L_LOGSECTION_APP_INIT, L_LOGLEVEL_TRACE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(bNewDownload)
	{
		if(!g_IsNeverDownloaded && !g_IsDownloadSuccessful)
		{
			// 下载过并且没有成功
			// 先删除后保存
			::DeleteFile(MyGetDefaultFolder() + g_downtask_xml_filename);
			CString szSaveName;
			if(!g_Real_SaveFileName.IsEmpty())
			{
				szSaveName.Format(_T("%s.~P2S"), g_Real_SaveFileName);
				SaveConfig(MyGetDefaultFolder() + g_downtask_xml_filename, g_DownLoadSaveFolder, szSaveName, g_wac_RealSaveFileName, g_IsDownloadSuccessful);
			}
		}
	}
	else
	{
	}

	__LOG_LOG__( _T("App uninit suceessful."), L_LOGSECTION_APP_UNINIT, L_LOGLEVEL_TRACE);
	//DeleteTray(g_hMainWnd);
	Clean();
	GdiplusShutdown(gdiplusToken);
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LIVEBOOTDOWNLOADER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= g_szWindowClass;
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   g_hMainWnd = CreateWindow(g_szWindowClass, StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), 
	   WS_OVERLAPPEDWINDOW &~ WS_MAXIMIZEBOX &~ WS_THICKFRAME,
	  CW_USEDEFAULT, 0, WINODW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

   if (!g_hMainWnd)
   {
	  return FALSE;
   }

   // center window
   CenterTopLevelWindow(g_hMainWnd);

   ShowWindow(g_hMainWnd, nCmdShow);
   UpdateWindow(g_hMainWnd);
   return TRUE;
}

//INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	case WM_INITDIALOG:
//		{
//			RECT r;
//			GetClientRect(g_hMainWnd, &r);
//			::MoveWindow(hDlg, r.left, r.top, r.right - r.left, r.bottom - r.top, FALSE);
//
//			// reset pos & size
//			{
//				::MoveWindow(GetDlgItem(hDlg, IDC_EDIT_SAVE_PATH),  g_rc_MainDlg_edit_savepath.X, g_rc_MainDlg_edit_savepath.Y, g_rc_MainDlg_edit_savepath.Width, g_rc_MainDlg_edit_savepath.Height, FALSE);
//				::MoveWindow(GetDlgItem(hDlg, IDC_BUTTON_BROWSE),  g_rc_MainDlg_btn_browse.X, g_rc_MainDlg_btn_browse.Y, g_rc_MainDlg_btn_browse.Width, g_rc_MainDlg_btn_browse.Height, FALSE);
//				::MoveWindow(GetDlgItem(hDlg, IDC_BTN_DOWNLOAD),  g_rc_MainDlg_btn_download.X, g_rc_MainDlg_btn_download.Y, g_rc_MainDlg_btn_download.Width, g_rc_MainDlg_btn_download.Height, FALSE);
//				::MoveWindow(GetDlgItem(hDlg, IDCANCEL),  g_rc_MainDlg_btn_cancel.X, g_rc_MainDlg_btn_cancel.Y, g_rc_MainDlg_btn_cancel.Width, g_rc_MainDlg_btn_cancel.Height, FALSE);
//			}
//
//			// set window text
//			{
//				::SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_BROWSE), StringFromIDResource(g_hInst, ID_TEXT_lang_MainDlg_btn_browse));
//				::SetWindowText(GetDlgItem(hDlg, IDC_BTN_DOWNLOAD), StringFromIDResource(g_hInst, ID_TEXT_lang_MainDlg_btn_download));
//				::SetWindowText(GetDlgItem(hDlg, IDCANCEL), StringFromIDResource(g_hInst, ID_TEXT_lang_MainDlg_btn_cancel));
//			}
//
//			// set font
//			{
//				// font
//				HFONT hFont = ::CreateFontIndirect(&g_font_text_default);
//				::SendMessage(GetDlgItem(hDlg, IDC_EDIT_SAVE_PATH), WM_SETFONT, (WPARAM)hFont, FALSE);
//			}
//
//			// set button control style to ownerdraw
//			//{
//			//	if(g_pBtnImage_Normal && g_pBtnImage_Hover && g_pBtnImage_Select && g_pBtnImage_Disable)
//			//	{
//			//		long style = GetWindowLong(GetDlgItem(hDlg, IDC_BTN_DOWNLOAD), GWL_STYLE);  
//			//		style = style | BS_OWNERDRAW;  
//			//		::SetWindowLong(GetDlgItem(hDlg, IDC_BTN_DOWNLOAD), GWL_STYLE, style);
//			//		::SetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_BROWSE), GWL_STYLE, style);
//			//		::SetWindowLong(GetDlgItem(hDlg, IDCANCEL), GWL_STYLE, style);
//
//			//		g_Main_btn_browse_proc = ::SetWindowLongPtr(GetDlgItem(hDlg, IDC_BUTTON_BROWSE), GWLP_WNDPROC, (LONG)Main_Btn_Browse_Proc);
//			//		g_Main_btn_download_proc = ::SetWindowLongPtr(GetDlgItem(hDlg, IDC_BTN_DOWNLOAD), GWLP_WNDPROC, (LONG)Main_Btn_Download_Proc);
//			//		g_Main_btn_cancel_proc = ::SetWindowLongPtr(GetDlgItem(hDlg, IDCANCEL), GWLP_WNDPROC, (LONG)Main_Btn_Cancel_Proc);
//			//	}
//			//}
//
//			{
//				// set & show default save path
//				::SetWindowText(GetDlgItem(hDlg, IDC_EDIT_SAVE_PATH), g_DownLoadSaveFolder);
//			}
//
//			if(g_pid_info.ad_page_urls_.size() > 0)
//			{
//				RECT r;
//				GetClientRect(hDlg, &r);
//				r.bottom -= 112;
//				IWebBrowser2* iWebBrowser;
//				VARIANT varMyURL;
//				//static CAxWindow WinContainer;
//				LPOLESTR pszName=OLESTR("shell.Explorer.2");
//				//g_MainDlgWinContainer.Create(hDlg, r, 0, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
//				g_MainDlgWinContainer.Create(hDlg, r, 0, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);//, WS_EX_CLIENTEDGE);
//				HRESULT hr = g_MainDlgWinContainer.CreateControl(pszName);
//				g_MainDlgWinCreated = TRUE;
//
//				ATLASSERT( SUCCEEDED(hr));
//				g_MainDlgWinContainer.QueryControl(__uuidof(IWebBrowser2),(void**)&iWebBrowser); 
//				VariantInit(&varMyURL);
//				varMyURL.vt = VT_BSTR; 
//				varMyURL.bstrVal = SysAllocString(g_pid_info.ad_page_urls_[0].c_str());
//				//varMyURL.bstrVal = SysAllocString(_T("http://10.10.24.83/downloading.html"));
//				hr = iWebBrowser->Navigate2(&varMyURL,0,0,0,0);
//				if (SUCCEEDED(hr))
//				{
//					iWebBrowser->put_Visible(VARIANT_TRUE);
//					//Sleep(300);
//					//wait for load complete
//					//DWORD dwCookie;
//					//hr = AtlAdvise(iWebBrowser, GetUnknown(), DIID_DWebBrowserEvents2, &dwCookie);
//					//IEEvt
//					//for(int i = 0; i < 200; ++i)
//					//{
//					//	READYSTATE ready = READYSTATE_UNINITIALIZED;
//					//	iWebBrowser->get_ReadyState(&ready);
//					//	if(ready == READYSTATE_COMPLETE)
//					//	{
//					//		break;
//					//	}
//					//	else
//					//	{
//					//		//Sleep(10);				
//					//	}
//					//}
//				}
//				else
//				{
//					iWebBrowser->Quit();
//				}
//				VariantClear(&varMyURL); 
//				iWebBrowser->Release();
//			}
//			break;
//		}
//	case WM_ERASEBKGND:
//		{
//			return TRUE;
//		}
//	case WM_PAINT:
//		{
//			PAINTSTRUCT ps;
//			HDC hdc = BeginPaint(hDlg, &ps);
//
//			if(g_pid_info.ad_page_urls_.size() > 0)
//			{
//				DrawMainDlgBk(hDlg, hdc, g_pEmptyDownloadDlgBKImage);
//			}
//			else
//			{
//				DrawMainDlgBk(hDlg, hdc, g_pDownloadDlgBKImage);
//			}
//			
//			//DrawDefaultBKImage(hDlg, hdc, g_pDefaultBKImage);
//			EndPaint(hDlg, &ps);
//			break;
//		}
//	case WM_DRAWITEM:
//		{
//			LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) lParam;
//			if(lpDIS->CtlType == ODT_BUTTON)
//			{
//				if(g_pid_info.ad_page_urls_.size() > 0)
//				{
//					DrawButton(1, g_pEmptyDownloadDlgBKImage, lpDIS);
//				}
//				else
//				{
//					DrawButton(1, g_pDownloadDlgBKImage, lpDIS);
//				}
//			}	
//		}
//		break;
//	case WM_CTLCOLOREDIT:
//		{
//			// edit control text color
//			::SetTextColor((HDC)wParam, COLOR_TEXT_DEFAULT);
//
//			// edit control bk brush
//			LOGBRUSH br;
//			br.lbStyle = BS_SOLID;
//			br.lbHatch = HS_CROSS;
//			br.lbColor = ::GetBkColor((HDC)wParam);
//			HBRUSH hBrush = ::CreateBrushIndirect(&br);
//			return (INT_PTR)hBrush;
//		}
//
//	case WM_COMMAND:
//		switch (LOWORD(wParam))
//		{
//		case IDCANCEL:
//		case IDOK:
//			{
//				if(IDYES == ::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_QUIT_while_not_downloading), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
//				{
//					PostQuitMessage(0);
//					return TRUE;
//				}
//				else
//				{
//					return TRUE;
//				}
//			}
//		case IDC_BTN_DOWNLOAD:
//			{
//				// get save folder
//				TCHAR szFolder[MAX_PATH] = {0};
//				::GetWindowText(GetDlgItem(hDlg, IDC_EDIT_SAVE_PATH), szFolder, MAX_PATH);
//				g_DownLoadSaveFolder.Format(_T("%s"), szFolder);
//				
//				// check input path is valid or not
//				g_DownLoadSaveFolder.Trim();
//				if(!IsFolderValid(g_DownLoadSaveFolder))
//				{
//					::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_Input_folder_invalid), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_OK | MB_ICONINFORMATION);
//					return TRUE;
//				}
//				
//				if(g_DownLoadSaveFolder.Right(1) != _T("\\"))
//				{
//					g_DownLoadSaveFolder += _T("\\");
//				}
//				if(!IsFolderExist(g_DownLoadSaveFolder))
//				{
//					// not exist
//					if(IDYES == ::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_Create_Folder), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
//					{
//						if(!CreateFolder(g_DownLoadSaveFolder))
//						{
//							::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_Create_Folder_Failed), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_OK | MB_ICONINFORMATION);
//							return TRUE;
//						}
//					}
//					else
//					{
//						return TRUE;
//					}
//				}
//
//				HWND hWnd = ::CreateDialog(g_hInst, (LPCTSTR)IDD_DIALOG_DOWNLOAD, g_hMainWnd, DownLoadDlgProc);
//				ShowWindow(hWnd, SW_SHOWNORMAL);
//
//				//只有有url才会创建网页控件
//				if(g_MainDlgWinCreated)
//				{
//					g_MainDlgWinContainer.DestroyWindow();
//					g_MainDlgWinCreated = FALSE;
//				}
//				EndDialog(hDlg, 0);
//				return TRUE;
//			}
//		case IDC_BUTTON_BROWSE:
//			{
//				CString szSaveFolder = ShowBrowseCommonDialog(g_hMainWnd, StringFromIDResource(g_hInst, ID_TEXT_lang_Browse_dialog_title));
//				if(!szSaveFolder.Trim().IsEmpty())
//				{
//					g_DownLoadSaveFolder = szSaveFolder.Trim();
//					::SetWindowText(GetDlgItem(hDlg, IDC_EDIT_SAVE_PATH), g_DownLoadSaveFolder);
//				}
//				return TRUE;
//			}
//		default:
//			break;
//		}//switch
//		break;
//	}
//	return ::DefWindowProc(hDlg, message, wParam, lParam);
//}

BOOL ProcessDownFinished(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, NsResultEnum eDownloadResult)
{
	SetUI_UpdateData(NULL);

	// 关闭之前弹出的提示框
	{
		HWND hWndPrevious = ::FindWindowEx(NULL, NULL, _T("#32770"), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title));
		if(hWndPrevious)
		{
			::SendMessage(hWndPrevious, WM_COMMAND, IDNO, NULL);
		}
	}

	if(g_ProductDownSucc == TRUE)
	{
		//下载正常并且成功
		g_task_track.download_end = CurrentFormatTime();
		g_task_track.download_status = _T("ok");

		g_IsDownloading = FALSE;
		g_IsDownloadSuccessful = TRUE;
		::SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, (int)100, 0);
		::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE), _T("100%"));
		//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_LEFTTIME_VALUE), _T("0 s"));

		//展示完成广告页
		if(g_pid_info.finish_page_urls_.size() > 0)
		{
			RECT r;
			GetClientRect(hDlg, &r);
			r.bottom -= 112;

			VARIANT varMyURL; 
			VariantInit(&varMyURL);
			varMyURL.vt = VT_BSTR; 

			varMyURL.bstrVal = SysAllocString(g_pid_info.finish_page_urls_[0].c_str());
			g_iwebBrowser2-> Navigate2(&varMyURL,0,0,0,0);
			//Sleep(300);
			VariantClear(&varMyURL); 
		}

		::ShowWindow(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE), SW_HIDE);
		::ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_HIDE);
		//::ShowWindow(GetDlgItem(hDlg, IDC_BUTTON_MIN), SW_HIDE);
		//::ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
		//DestroyWindow(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE));
		//DestroyWindow(GetDlgItem(hDlg, IDC_PROGRESS));
		//DestroyWindow(GetDlgItem(hDlg, IDC_BUTTON_MIN));
		//DestroyWindow(GetDlgItem(hDlg, IDCANCEL));

		//下载完成设置为显示
		::ShowWindow(GetDlgItem(hDlg, IDC_INSTALL), SW_SHOWNORMAL);

		RECT r;
		GetClientRect(hDlg, &r);
		r.top += 264;
		InvalidateRect(hDlg, &r, TRUE);
		//UpdateWindow(hDlg);
					
		// remove task xml file
		::DeleteFile(MyGetDefaultFolder() + g_downtask_xml_filename);
		SaveConfig(MyGetDefaultFolder() + g_downtask_xml_filename, g_DownLoadSaveFolder, g_Real_SaveFileName, g_wac_RealSaveFileName, g_IsDownloadSuccessful);

		//让最小化窗口闪动
		if(IsIconic(g_hMainWnd))
		{
			FlashWindow(g_hMainWnd, FALSE);
		}
		//if(!IsWindowVisible(g_hMainWnd))
		//{
		//	SetTrayBalloonTip(g_hMainWnd, StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title).GetBuffer(), _T("Download finished, Click to install"));
		//}
		
		//取消显示主界面
		//::ShowWindow(g_hMainWnd, SW_SHOWNORMAL);
		//if(IDYES == ::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_Download_Complete), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
		//{
		//	g_task_track.install_start = CurrentFormatTime();

		//	// run the app
		//	if(g_need_download_box && g_wac_DownSucc)
		//	{
		//		RunExe(g_DownLoadSaveFolder + g_wac_RealSaveFileName, NULL, true, true);
		//	}
		//	int rtnCode = RunExe(g_DownLoadSaveFolder + g_Real_SaveFileName, NULL,false,true);

		//	g_task_track.install_end = CurrentFormatTime();
		//	if(rtnCode == 0)
		//	{
		//		g_task_track.install_status = _T("ok");
		//	}
		//	else
		//	{
		//		g_task_track.install_status = _T("fail");
		//	}
		//	SendInstallMessage(g_ProductPID,rtnCode);
		//}

		// quit
		//ReportTrackInfoToServer();
		//cancel
		//PostQuitMessage(0);
	}
	else
	{
		//下载失败
		g_task_track.download_end = CurrentFormatTime();
		g_task_track.download_status = _T("fail");

		g_IsDownloading = FALSE;
		g_IsDownloadSuccessful = FALSE;
		::ShowWindow(g_hMainWnd, SW_SHOWNORMAL);

		// md5 check failed ?
		BOOL bRetry = FALSE;
		if(eDownloadResult == RS_ERR_MD5_CHECK_FAILED)
		{
			if(IDRETRY == ::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_Download_md5_check_Failed), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_RETRYCANCEL | MB_ICONERROR))
			{
				bRetry = TRUE;
			}

			::DeleteFile(g_DownLoadSaveFolder + g_Real_SaveFileName);
			::DeleteFile(MyGetDefaultFolder() + g_downtask_xml_filename);
		}
		else if(eDownloadResult == RS_ERR_DISK_SPACE_NOT_ENOUGH)
		{
			//加上盘符提示
			CString disk = g_DownLoadSaveFolder.Left(1);
			::MessageBox(g_hMainWnd, disk + _T(" ") + StringFromIDResource(g_hInst, ID_MSG_lang_Download_disk_space_not_enough), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_OK | MB_ICONERROR);

			{
				//只有有url才会创建网页控件
				if(g_DownDlgWinCreated)
				{
					g_DownDlgWinContainer.DestroyWindow();
					g_DownDlgWinCreated = FALSE;
				}
				
				EndDialog(hDlg, 0);
				//HWND hWnd = ::CreateDialog(g_hInst, (LPCTSTR)IDD_DIALOG_MAIN, g_hMainWnd, MainDlgProc);
				HWND hWnd = ::CreateDialog(g_hInst, (LPCTSTR)IDD_DIALOG_DOWNLOAD, g_hMainWnd, DownLoadDlgProc);
				ShowWindow(hWnd, SW_SHOWNORMAL);

				//HWND hChildHwnd = FindWindowEx(hWnd, NULL, _T("Edit"), NULL);
				//if(hChildHwnd)
				//{
				//	::SetWindowText(GetDlgItem(hChildHwnd, IDC_EDIT_SAVE_PATH), g_DownLoadSaveFolder);
				//}
				return TRUE;
			}
		}
		else
		{
			CString sz;
			int nErrCode = int(eDownloadResult);
			if(eDownloadResult == RS_ERR_CONNECT_FAILED || 
				eDownloadResult == RS_ERR_RCV_FAILED ||
				eDownloadResult == RS_ERR_SEND_FAILED ||
				eDownloadResult == RS_ERR_GET_WEBINFO_FAILED)
			{
				sz = StringFromIDResource(g_hInst, ID_MSG_lang_Download_network_connect_Failed);
			}
			else
			{
				sz.Format(_T("%s\r\nError code : 0x%08x"), StringFromIDResource(g_hInst, ID_MSG_lang_Download_Failed), nErrCode);
			}
			if(IDRETRY == ::MessageBox(g_hMainWnd, sz, StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_RETRYCANCEL | MB_ICONERROR))
			{
				bRetry = TRUE;
			}
		}

		if(bRetry)
		{
			//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_SPEED_VALUE), _T("0 Bytes/s"));
			//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_LEFTTIME_VALUE), _T(""));

			if(g_pFunctionInterface)
			{
			///////////////////////////////////////////////////////////////////////
				//reset status
				g_task_track.download_end = _T("");
				g_task_track.download_start = _T("");
				g_task_track.download_status = _T("unknown");

				g_pFunctionInterface->SetCallBack((ns_WAC::IDownloaderManagerCallBack*)NULL);
				g_download_task_id = -1;
				g_wac_DownSucc = -1;
				g_ProductDownSucc = -1;
				g_wac_DownSucc = -1;
				//只有有url才会创建网页控件
				if(g_DownDlgWinCreated)
				{
					g_DownDlgWinContainer.DestroyWindow();
					g_DownDlgWinCreated = FALSE;
				}
				::KillTimer(hDlg, 999);
				//::KillTimer(hDlg, 998);
				EndDialog(hDlg, 0);

				//////////////////////////////////////////////////////////////////////////
				//load xml file from server,可以得到产品名称，广告页面url,box下载url
				RequestPIDInfo();
				if(g_pid_info.product_name_ == _T(""))
				{
					g_pid_info.product_name_ = DEFAULT_WINDOWS_TITLE;
				}

				//HWND hWnd = ::CreateDialog(g_hInst, (LPCTSTR)IDD_DIALOG_MAIN, g_hMainWnd, MainDlgProc);
				HWND hWnd = ::CreateDialog(g_hInst, (LPCTSTR)IDD_DIALOG_DOWNLOAD, g_hMainWnd, DownLoadDlgProc);

				ShowWindow(hWnd, SW_SHOWNORMAL);
								

				//HWND hChildHwnd = FindWindowEx(hWnd, NULL, _T("Edit"), NULL);
				//if(hChildHwnd)
				//{
				//	::SetWindowText(GetDlgItem(hChildHwnd, IDC_EDIT_SAVE_PATH), g_DownLoadSaveFolder);
				//}
				return TRUE;
			}
			else
			{
				ATLASSERT(0);
			}
		}
		else
		{
			// quit
			ReportTrackInfoToServer();
			PostQuitMessage(0);
		}
	}

	return TRUE;
}

INT_PTR CALLBACK DownLoadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	//MSG *lpMsg; 
	//lpMsg = (MSG*)lParam; 
	//if (message>= 0) 
	//{ 
	//	if(lpMsg-> message == WM_RBUTTONDOWN) 
	//	{ 
	//		//MessageBox( _T("Get Mouse right button down event ")); 
	//		return 0;//PostQuitMessage(0); 
	//	} 
	//}

	if(message == g_nMessageID)
	{
		t_DownloadNotifyPara *pdlntfData = (t_DownloadNotifyPara*)lParam;
		if(pdlntfData->nSerialNum == g_wac_task_id)
		{
			CString csStatus;
			int nSerialNum = -1;
			switch ( pdlntfData->nNotityType )
			{
				case NOTIFY_TYPE_HAS_GOT_REMOTE_FILENAME:
				{
					CString szFileName((wchar_t*)pdlntfData->lpNotifyData);
					g_wac_RealSaveFileName.Format(_T("%s"), szFileName);
					break;
				}
				//获得服务器端文件大小
				case NOTIFY_TYPE_HAS_GOT_REMOTE_FILESIZE:
				{
					g_product_filesize = (__int64)pdlntfData->lpNotifyData;
					break;
				}
				//下载结束
				case NOTIFY_TYPE_HAS_END_DOWNLOAD:
				{
					//如果成功，看产品下载是否结束，否则置标志位后返回
					NsResultEnum eDownloadResult = (NsResultEnum)(int)pdlntfData->lpNotifyData;
					if(eDownloadResult == RS_SUCCESS)
					{
						g_wac_DownSucc = TRUE;
					}
					else
					{
						g_wac_DownSucc = FALSE;
					}

					if(g_ProductDownSucc != -1)
					{
						ProcessDownFinished(hDlg, message, wParam, lParam, g_product_download_result);
					}

					break;
				}
				//下载过程信息
				case NOTIFY_TYPE_HAS_GOT_NEW_DOWNLOADED_DATA:
					{
						NotifyData *pInfo = (NotifyData*)pdlntfData->lpNotifyData;
						SetUI_UpdateData(pInfo);
						delete pInfo;
						pInfo = NULL;
						break;
					}
				default:
					break;
			}
			return TRUE;
		}
		else if(pdlntfData->nSerialNum == g_download_task_id)
		{

			CString csStatus;
			int nSerialNum = -1;
			switch ( pdlntfData->nNotityType )
			{
				//获得文件名字
			case NOTIFY_TYPE_HAS_GOT_REMOTE_FILENAME:
				{
					CString szFileName((wchar_t*)pdlntfData->lpNotifyData);
					g_Real_SaveFileName.Format(_T("%s"), szFileName);
					break;
				}
				//获得服务器端文件大小
			case NOTIFY_TYPE_HAS_GOT_REMOTE_FILESIZE:
				{
					g_wac_filesize = (__int64)pdlntfData->lpNotifyData;
					//CString sz = FormatSize(nTotalSize);
					//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_FILE_SIZE_VALUE), sz);
					break;
				}
				//下载结束
			case NOTIFY_TYPE_HAS_END_DOWNLOAD:
				{
					NsResultEnum eDownloadResult = (NsResultEnum)(int)pdlntfData->lpNotifyData;
					if ( eDownloadResult == RS_SUCCESS)
					{
						g_ProductDownSucc = TRUE;
					}
					else
					{
						g_ProductDownSucc = FALSE;
						g_product_download_result = eDownloadResult;
					}

					if(g_wac_DownSucc != -1 || g_need_download_box == FALSE)
					{
						ProcessDownFinished(hDlg, message, wParam, lParam, eDownloadResult);
					}
					
					break;
				}
				//下载过程信息
			case NOTIFY_TYPE_HAS_GOT_NEW_DOWNLOADED_DATA:
				{
					NotifyData *pInfo = (NotifyData*)pdlntfData->lpNotifyData;
					SetUI_UpdateData(pInfo);
	/*
					__int64 nCurPos = pInfo->cbCurPos;
					__int64 nTotalSize = pInfo->cbTotle;
					int nSpeed = pInfo->cbSpeedPerSecond;

					CString szTemp;
					//speed
					szTemp = FormatSpeed(nSpeed);
					::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_SPEED_VALUE), szTemp);

					// left time
					szTemp = FormatLeftTime(nSpeed, nTotalSize - nCurPos);
					::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_LEFTTIME_VALUE), szTemp);

					// downloaded size
					szTemp = FormatSize(nCurPos);
					::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DOWNLOADED_SIZE_VALUE), szTemp);

					//progress
					double nPercent = (nCurPos * 100.00) / nTotalSize;
					szTemp.Format(_T("%d"), (int)nPercent);
					szTemp += _T("%");
					::SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, (int)nPercent, 0);
					::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE), szTemp);
	*/

					delete pInfo;
					pInfo = NULL;
					break;
				}
				// 获得某线程将要下载的文件大小
			case NOTIFY_TYPE_HAS_GOT_THREAD_WILL_DOWNLOAD_SIZE:
				{
					break;
				}
				// 获得某线程已经下载的文件大小
			case NOTIFY_TYPE_HAS_GOT_THREAD_DOWNLOADED_SIZE:
				{
					break;
				}
				// 获得某线程已经下载的缓存数据大小
			case NOTIFY_TYPE_HAS_GOT_THREAD_CACHE_SIZE:
				{
					break;
				}
			default:
				break;
			}
			return TRUE;
		}
		else
		{
			ATLASSERT(0);
			return TRUE;
		}
	}

	switch (message)
	{
	case WM_INITDIALOG:
		{
			RECT r;
			::GetClientRect(g_hMainWnd, &r);
			::MoveWindow(hDlg,  r.left, r.top, r.right - r.left, r.bottom - r.top, FALSE);

			//直接进入安装页面
			if(g_into_install)
			{
				::MoveWindow(GetDlgItem(hDlg, IDC_INSTALL), g_rc_DownLoadDlg_install.X, g_rc_DownLoadDlg_install.Y, g_rc_DownLoadDlg_install.Width, g_rc_DownLoadDlg_install.Height, FALSE);
				////设置install按钮的字体
				//// font
				HFONT hFont = NULL;
				hFont = ::CreateFontIndirect(&g_font_install_btn_default);
				::SendDlgItemMessage(hDlg, IDC_INSTALL, WM_SETFONT, (WPARAM)hFont, TRUE);

				::SetWindowText(GetDlgItem(hDlg, IDC_INSTALL), StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_btn_install));

				{
					if(g_pid_info.finish_page_urls_.size() > 0)
					{
						RECT r;
						GetClientRect(hDlg, &r);
						r.bottom -= 112;
						VARIANT varMyURL;
						//static CAxWindow WinContainer;
						LPOLESTR pszName=OLESTR("shell.Explorer.2");
						g_DownDlgWinContainer.Create(hDlg, r, 0, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);//, WS_EX_CLIENTEDGE);
						g_DownDlgWinCreated = TRUE;

						HRESULT hr = g_DownDlgWinContainer.CreateControl(pszName);
						g_DownDlgWinContainer.QueryControl(__uuidof(IWebBrowser2),(void**)&g_iwebBrowser2); 
						VariantInit(&varMyURL);
						varMyURL.vt = VT_BSTR; 
						varMyURL.bstrVal = SysAllocString(g_pid_info.finish_page_urls_[0].c_str());
						g_iwebBrowser2->Navigate2(&varMyURL,0,0,0,0);
						if (SUCCEEDED(hr))
						{
							g_iwebBrowser2->put_Visible(VARIANT_TRUE);
							//Sleep(300);
							//for(int i = 0; i < 200; ++i)
							//{
							//	READYSTATE ready = READYSTATE_UNINITIALIZED;
							//	g_iwebBrowser2->get_ReadyState(&ready);
							//	if(ready == READYSTATE_COMPLETE)
							//	{
							//		break;
							//	}
							//	else
							//	{
							//		//Sleep(10);				
							//	}
							//}
						}
						else
						{
							g_iwebBrowser2->Quit();
						}
						VariantClear(&varMyURL); 
					}
				}
				g_IsDownloading = FALSE;
				g_IsNeverDownloaded = TRUE;
				g_IsDownloadSuccessful = TRUE;
				g_wac_DownSucc = TRUE;
				g_ProductDownSucc = TRUE;
				::ShowWindow(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE), SW_HIDE);
				::ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_HIDE);
				//::ShowWindow(GetDlgItem(hDlg, IDC_BUTTON_MIN), SW_HIDE);
				//::ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);

				//下载完成设置为显示
				::ShowWindow(GetDlgItem(hDlg, IDC_INSTALL), SW_SHOWNORMAL);

				RECT r;
				GetClientRect(hDlg, &r);
				r.top += 264;
				InvalidateRect(hDlg, &r, TRUE);
				break;
			}

			::SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			::SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, (int)0, 0);
			::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE), _T("0%"));
			//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_FILE_SIZE_VALUE), _T("Unknown"));
			//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DOWNLOADED_SIZE_VALUE), _T("0 Bytes"));
			//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_SPEED_VALUE), _T("0 Bytes/s"));
			//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_LEFTTIME_VALUE), _T(""));

			// reset pos & size
			{
				//::MoveWindow(GetDlgItem(hDlg, IDC_STATIC_SPEED_VALUE),  g_rc_DownLoadDlg_static_speed_value.X, g_rc_DownLoadDlg_static_speed_value.Y, g_rc_DownLoadDlg_static_speed_value.Width, g_rc_DownLoadDlg_static_speed_value.Height, FALSE);
				//::MoveWindow(GetDlgItem(hDlg, IDC_STATIC_LEFTTIME_VALUE),  g_rc_DownLoadDlg_static_lefttime_value.X, g_rc_DownLoadDlg_static_lefttime_value.Y, g_rc_DownLoadDlg_static_lefttime_value.Width, g_rc_DownLoadDlg_static_lefttime_value.Height, FALSE);
				::MoveWindow(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE),  g_rc_DownLoadDlg_static_progress.X, g_rc_DownLoadDlg_static_progress.Y, g_rc_DownLoadDlg_static_progress.Width, g_rc_DownLoadDlg_static_progress.Height, FALSE);
				//::MoveWindow(GetDlgItem(hDlg, IDC_STATIC_FILE_SIZE_VALUE),  g_rc_DownLoadDlg_static_filesize_value.X, g_rc_DownLoadDlg_static_filesize_value.Y, g_rc_DownLoadDlg_static_filesize_value.Width, g_rc_DownLoadDlg_static_filesize_value.Height, FALSE);
				//::MoveWindow(GetDlgItem(hDlg, IDC_STATIC_DOWNLOADED_SIZE_VALUE),  g_rc_DownLoadDlg_static_remainingsize_value.X, g_rc_DownLoadDlg_static_remainingsize_value.Y, g_rc_DownLoadDlg_static_remainingsize_value.Width, g_rc_DownLoadDlg_static_remainingsize_value.Height, FALSE);
				//::MoveWindow(GetDlgItem(hDlg, IDC_BUTTON_MIN),  g_rc_DownLoadDlg_btn_min.X, g_rc_DownLoadDlg_btn_min.Y, g_rc_DownLoadDlg_btn_min.Width, g_rc_DownLoadDlg_btn_min.Height, FALSE);
				//::MoveWindow(GetDlgItem(hDlg, IDCANCEL),  g_rc_DownLoadDlg_btn_cancel.X, g_rc_DownLoadDlg_btn_cancel.Y, g_rc_DownLoadDlg_btn_cancel.Width, g_rc_DownLoadDlg_btn_cancel.Height, FALSE);
				::MoveWindow(GetDlgItem(hDlg, IDC_PROGRESS),  g_rc_DownLoadDlg_progress.X, g_rc_DownLoadDlg_progress.Y, g_rc_DownLoadDlg_progress.Width, g_rc_DownLoadDlg_progress.Height, FALSE);
				//设置安装按钮位置
				::MoveWindow(GetDlgItem(hDlg, IDC_INSTALL), g_rc_DownLoadDlg_install.X, g_rc_DownLoadDlg_install.Y, g_rc_DownLoadDlg_install.Width, g_rc_DownLoadDlg_install.Height, FALSE);
			
			}

			//////////////////////////////////////////////////////////////////////////
			// set window text
			{
				//::SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_MIN), StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_btn_min));
				//::SetWindowText(GetDlgItem(hDlg, IDCANCEL), StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_btn_cancel));				
				//设置安装按钮文字

				////设置install按钮的字体
				//// font
				HFONT hFont = NULL;
				hFont = ::CreateFontIndirect(&g_font_install_btn_default);
				::SendDlgItemMessage(hDlg, IDC_INSTALL, WM_SETFONT, (WPARAM)hFont, TRUE);

				::SetWindowText(GetDlgItem(hDlg, IDC_INSTALL), StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_btn_install));
			}

			// set static control style to ownerdraw
			{
				long style = GetWindowLong(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE), GWL_STYLE);  
				style = style | SS_OWNERDRAW;  
				//::SetWindowLong(GetDlgItem(hDlg, IDC_STATIC_SPEED_VALUE), GWL_STYLE, style);
				//::SetWindowLong(GetDlgItem(hDlg, IDC_STATIC_LEFTTIME_VALUE), GWL_STYLE, style);
				::SetWindowLong(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE), GWL_STYLE, style);
				//::SetWindowLong(GetDlgItem(hDlg, IDC_STATIC_FILE_SIZE_VALUE), GWL_STYLE, style);
				//::SetWindowLong(GetDlgItem(hDlg, IDC_STATIC_DOWNLOADED_SIZE_VALUE), GWL_STYLE, style);
			}

			// set button control style to ownerdraw
			//{
			//	if(g_pBtnImage_Normal && g_pBtnImage_Hover && g_pBtnImage_Select && g_pBtnImage_Disable)
			//	{
			//		long style = GetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_MIN), GWL_STYLE);  
			//		style = style | BS_OWNERDRAW;  
			//		//::SetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_MIN), GWL_STYLE, style);
			//		//::SetWindowLong(GetDlgItem(hDlg, IDCANCEL), GWL_STYLE, style);
			//		::SetWindowLong(GetDlgItem(hDlg, IDC_INSTALL), GWL_STYLE, style);

			//		//g_DownLoad_btn_min_proc = ::SetWindowLongPtr(GetDlgItem(hDlg, IDC_BUTTON_MIN), GWLP_WNDPROC, (LONG)DownLoad_Btn_Min_Proc);
			//		//g_DownLoad_btn_cancel_proc = ::SetWindowLongPtr(GetDlgItem(hDlg, IDCANCEL), GWLP_WNDPROC, (LONG)DownLoad_Btn_Cancel_Proc);
			//		//g_DownLoad_btn_install_proc = ::SetWindowLongPtr(GetDlgItem(hDlg, IDC_INSTALL), GWLP_WNDPROC, (LONG)DownLoad_Btn_Install_Proc);
			//	}
			//}

			//未下载完成设置为隐藏
			::ShowWindow(GetDlgItem(hDlg, IDC_INSTALL), SW_HIDE);
			//::ShowWindow(GetDlgItem(hDlg, IDC_BUTTON_MIN), SW_HIDE);
			//::ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);


			{
				if(g_pid_info.ad_page_urls_.size() > 0)
				{
					RECT r;
					GetClientRect(hDlg, &r);
					r.bottom -= 112;
					VARIANT varMyURL;
					//static CAxWindow WinContainer;
					LPOLESTR pszName=OLESTR("shell.Explorer.2");
					g_DownDlgWinContainer.Create(hDlg, r, 0, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);//, WS_EX_CLIENTEDGE);
					g_DownDlgWinCreated = TRUE;

					HRESULT hr = g_DownDlgWinContainer.CreateControl(pszName);
					g_DownDlgWinContainer.QueryControl(__uuidof(IWebBrowser2),(void**)&g_iwebBrowser2); 
					VariantInit(&varMyURL);
					varMyURL.vt = VT_BSTR; 
					varMyURL.bstrVal = SysAllocString(g_pid_info.ad_page_urls_[g_ShowUrlSeq++].c_str());
					if(g_ShowUrlSeq >= (int)g_pid_info.ad_page_urls_.size())
					{
						g_ShowUrlSeq = 0;
					}
					hr = g_iwebBrowser2->Navigate2(&varMyURL,0,0,0,0);
					if (SUCCEEDED(hr))
					{
						g_iwebBrowser2->put_Visible(VARIANT_TRUE);
						//Sleep(300);
						//for(int i = 0; i < 200; ++i)
						//{
						//	READYSTATE ready = READYSTATE_UNINITIALIZED;
						//	g_iwebBrowser2->get_ReadyState(&ready);
						//	if(ready == READYSTATE_COMPLETE)
						//	{
						//		break;
						//	}
						//	else
						//	{
						//		//Sleep(10);				
						//	}
						//}
					}
					else
					{
						g_iwebBrowser2->Quit();
					}
					VariantClear(&varMyURL); 
				}
			}

			// add task
			if(g_pFunctionInterface)
			{
				g_pFunctionInterface->SetCallBack(hDlg, g_nMessageID);

				g_download_task_id = g_pFunctionInterface->AddTask(g_DownloadURL.GetBuffer(g_DownloadURL.GetLength()), 
					g_DownLoadSaveFolder.GetBuffer(g_DownLoadSaveFolder.GetLength()), 
					_T(""), g_ProductPID);
				if(g_download_task_id < 0)
				{
					//error
				}
				if(g_need_download_box && g_wac_url.GetLength() > 0)
				{
					g_wac_task_id = g_pFunctionInterface->AddTask(g_wac_url.GetBuffer(g_wac_url.GetLength()),
										g_DownLoadSaveFolder.GetBuffer(g_DownLoadSaveFolder.GetLength()),
										_T(""), g_WacPID);
					//添加任务失败，则不下载box
					if(g_wac_task_id < 0)
					{
						g_wac_DownSucc = FALSE;
					}
				}
				
				g_IsDownloading = TRUE;
				g_IsNeverDownloaded = FALSE;
				g_IsDownloadSuccessful = FALSE;
			}

			// add timer
			{
				::SetTimer(hDlg, 999, 2000, (TIMERPROC) NULL);
				SetUI_UpdateData(NULL);
				//::SetTimer(hDlg, 998, 3000, (TIMERPROC)NULL);
			}

			//初始化g_task_track，开始下载
			g_task_track.download_start = CurrentFormatTime();
			g_task_track.download_status = _T("unknown");
			g_task_track.install_status = _T("unknown");
			break;
		}
	case WM_DESTROY:
		{
			if(g_need_download_box)
			{
				g_pFunctionInterface->RemoveTask(g_wac_task_id);
			}
			
			g_pFunctionInterface->RemoveTask(g_download_task_id);
			::KillTimer(hDlg, 999);
			//::KillTimer(hDlg, 998);
			if(g_iwebBrowser2 != NULL)
			{
				g_iwebBrowser2->Release();
			}

			g_task_track.download_end = CurrentFormatTime();
			g_task_track.download_status = _T("pause");
			ReportTrackInfoToServer();
			return 0;
		}
	case WM_TIMER:
		{
			/*if(wParam == 998)
			{
			if(g_pid_info.urls_.size() > 0)
			{
			RECT r;
			GetClientRect(hDlg, &r);
			r.bottom -= 150;

			VARIANT varMyURL; 
			VariantInit(&varMyURL);
			varMyURL.vt = VT_BSTR; 

			varMyURL.bstrVal = SysAllocString(g_pid_info.urls_[g_ShowUrlSeq++].c_str());
			if(g_ShowUrlSeq >= (int)g_pid_info.urls_.size())
			{
			g_ShowUrlSeq = 0;
			}

			g_iwebBrowser2-> Navigate2(&varMyURL,0,0,0,0);
			VariantClear(&varMyURL); 
			}

			}
			else*/ if(wParam == 999)
			{
				UI_UpdateData Item;
				GetUI_UpdateData(Item);
				if(Item.m_Valid)
				{
					__int64 nCurPos = Item.m_Info.cbCurPos;
					__int64 nTotalSize = Item.m_Info.cbTotle;
					int nSpeed = Item.m_Info.cbSpeedPerSecond;

					CString szTemp;
					////speed
					//szTemp = FormatSpeed(nSpeed);
					//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_SPEED_VALUE), szTemp);

					//// left time
					//szTemp = FormatLeftTime(nSpeed, nTotalSize - nCurPos);
					//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_LEFTTIME_VALUE), szTemp);

					//// downloaded size
					//szTemp = FormatSize(nCurPos);
					//::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DOWNLOADED_SIZE_VALUE), szTemp);

					//progress
					//保存当前的进度，用于保证进度不回退
					static double sPercent = 0;
					double nPercent = (nCurPos * 100.00) / nTotalSize;
					if(nPercent >= sPercent)
					{
						sPercent = nPercent;
					}
					szTemp.Format(_T("%d"), (int)sPercent);
					szTemp += _T("%");
					::SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, (int)sPercent, 0);
					::SetWindowText(GetDlgItem(hDlg, IDC_STATIC_PROGRESS_VALUE), szTemp);
				}
			}
			return 0;
		}
	case WM_ERASEBKGND:
		{
			return TRUE;
		}
	//case WM_NCPAINT:
	//	{
	//		PAINTSTRUCT ps;
	//		HDC hdc = BeginPaint(hDlg, &ps);
	//		DrawDownLoadDlgBk(hDlg, hdc, g_pDownloadDlgBKImage);
	//		//if(g_pid_info.ad_page_urls_.size() == 0)
	//		//{
	//		//	DrawDefaultBKImage(hDlg, hdc, g_pDefaultBKImage);
	//		//}	
	//		
	//		EndPaint(hDlg, &ps);

	//		//////////////////////////////////////////////////////////////////////////
	//		// font
	//		//HFONT hFont = NULL;
	//		//HWND hInstallBtn = GetDlgItem(hDlg, IDC_INSTALL);
	//		//RECT btnRect;
	//		//::GetWindowRect(hInstallBtn, &btnRect);
	//		//hFont = ::CreateFontIndirect(&g_font_install_btn_default);

	//		//HDC hdcBtn = GetDC(hInstallBtn);
	//		//if(hFont)
	//		//{
	//		//	::SelectObject(hdcBtn, hFont);
	//		//}

	//		//::DeleteObject(hFont);


	//		//// color
	//		//::SetTextColor(hdcBtn, COLOR_TEXT_DEFAULT);

	//		//// draw text
	//		//::SetBkMode(hdcBtn, TRANSPARENT);
	//		//TCHAR lpString[255] = {0};
	//		//::GetWindowText(hInstallBtn, lpString, sizeof(lpString) / sizeof(lpString[0]));
	//		//::DrawText(hdcBtn, lpString, wcslen(lpString), &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//		//ReleaseDC(hInstallBtn, hdcBtn);
	//		////////////////////////////////////////////////////////////////////////////
	//		break;
	//	}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);
			if(g_pid_info.ad_page_urls_.size() > 0)
			{
				DrawDownLoadDlgBk(hDlg, hdc, g_pEmptyDownloadDlgBKImage);
			}
			else
			{
				DrawDownLoadDlgBk(hDlg, hdc, g_pDownloadDlgBKImage);
			}
			
			//if(g_pid_info.ad_page_urls_.size() == 0)
			//{
			//	DrawDefaultBKImage(hDlg, hdc, g_pDefaultBKImage);
			//}	

			EndPaint(hDlg, &ps);

			//////////////////////////////////////////////////////////////////////////
			// font
			//HFONT hFont = NULL;
			//HWND hInstallBtn = GetDlgItem(hDlg, IDC_INSTALL);
			//RECT btnRect;
			//::GetWindowRect(hInstallBtn, &btnRect);
			//hFont = ::CreateFontIndirect(&g_font_install_btn_default);

			//HDC hdcBtn = GetDC(hInstallBtn);
			//if(hFont)
			//{
			//	::SelectObject(hdcBtn, hFont);
			//}

			//// color
			//::SetTextColor(hdcBtn, COLOR_TEXT_DEFAULT);

			//// draw text
			//::SetBkMode(hdcBtn, TRANSPARENT);
			//TCHAR lpString[255] = {0};
			//::GetWindowText(hInstallBtn, lpString, sizeof(lpString) / sizeof(lpString[0]));
			//::DrawText(hdcBtn, lpString, wcslen(lpString), &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			//ReleaseDC(hInstallBtn, hdcBtn);
			//::DeleteObject(hFont);
			////////////////////////////////////////////////////////////////////////////
			break;
		}
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) lParam;
			if(lpDIS->CtlType == ODT_STATIC)
			{
				if(g_pid_info.ad_page_urls_.size() > 0)
				{
					DrawStatic(g_pEmptyDownloadDlgBKImage, lpDIS);
				}
				else
				{
					DrawStatic(g_pDownloadDlgBKImage, lpDIS);
				}
			}
			//else if(lpDIS->CtlType == ODT_BUTTON)
			//{
			//	if(g_pid_info.ad_page_urls_.size() > 0)
			//	{
			//		DrawButton(2, g_pEmptyDownloadDlgBKImage, lpDIS);
			//	}
			//	else
			//	{
			//		DrawButton(2, g_pDownloadDlgBKImage, lpDIS);
			//	}
			//}
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		case IDOK:
			{
				BOOL bExit = FALSE;
				if(g_IsDownloading)
				{
					if(IDYES == ::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_QUIT_while_downloading), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
					{
						bExit = TRUE;
					}
				}
				else
				{
					if(IDYES == ::MessageBox(g_hMainWnd, StringFromIDResource(g_hInst, ID_MSG_lang_QUIT_while_not_downloading), StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title), MB_YESNO | MB_ICONINFORMATION))
					{
						bExit = TRUE;
					}
				}


				if(bExit)
				{
					// quit
					::KillTimer(hDlg, 999);
					//::KillTimer(hDlg, 998);
					if(g_need_download_box)
					{
						g_pFunctionInterface->RemoveTask(g_wac_task_id);
					}
					
					g_pFunctionInterface->RemoveTask(g_download_task_id);


					g_task_track.download_end = CurrentFormatTime();
					g_task_track.download_status = _T("pause");
					ReportTrackInfoToServer();
					PostQuitMessage(0);
					return TRUE;
				}
				else
				{
					return TRUE;
				}
			}
		//case IDC_BUTTON_MIN:
		//	{
		//		::ShowWindow(g_hMainWnd, SW_HIDE);

		//		CreateTray(g_hMainWnd, StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title).GetBuffer());
		//		SetTrayBalloonTip(g_hMainWnd, StringFromIDResource(g_hInst, ID_TEXT_lang_Window_title).GetBuffer(), StringFromIDResource(g_hInst, ID_MSG_lang_Tray_msg).GetBuffer());
		//		return TRUE;
		//	}
		case IDC_INSTALL:
			{
				ShowWindow(g_hMainWnd, SW_HIDE);
				UpdateWindow(g_hMainWnd);

				g_task_track.install_start = CurrentFormatTime();

				
				//int rtnCode = RunExe(g_DownLoadSaveFolder + g_Real_SaveFileName, NULL,false,true);
			
				//// run the app
				//if(g_need_download_box && g_wac_DownSucc == TRUE)
				//{
				//	RunExe(g_DownLoadSaveFolder + g_wac_RealSaveFileName, NULL, true, false);
				//}

				//改为先安装box，然后安装产品，然后重启服务			
				// run the app
				if(g_need_download_box && g_wac_DownSucc == TRUE)
				{
					RunExe(g_DownLoadSaveFolder + g_wac_RealSaveFileName, NULL, true, true);
				}

				// install product
				int rtnCode = RunExe(g_DownLoadSaveFolder + g_Real_SaveFileName, NULL,false,true);

				//restart wacService
				int service_rtnCode = RunCmd(_T("sc"), _T("stop WACService"), true, true);
				CString szLog;
				szLog.Format(_T("stop WACService return: %d"), service_rtnCode);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

				service_rtnCode = RunCmd(_T("sc"), _T("start WACService"), true, true);
				szLog.Format(_T("start WACService return: %d"), service_rtnCode);
				__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);


				g_task_track.install_end = CurrentFormatTime();
				if(rtnCode == 0)
				{
					g_task_track.install_status = _T("ok");
				}
				else
				{
					g_task_track.install_status = _T("fail");
				}
				SendInstallMessage(g_ProductPID,rtnCode);

				ReportTrackInfoToServer();
				PostQuitMessage(0);
				return TRUE;
			}
		default:
			break;
		}//switch
		break;
	}
	return ::DefWindowProc(hDlg, message, wParam, lParam);
}


void DrawStatic(Image *pBKImage, LPDRAWITEMSTRUCT lpDIS)
{
	HDC hdc = lpDIS->hDC;

	// 控件的宽和高
	int width = lpDIS->rcItem.right - lpDIS->rcItem.left;
	int height = lpDIS->rcItem.bottom - lpDIS->rcItem.top;

	// 获取该控件在父窗口中的坐标
	RECT r;
	::GetWindowRect(lpDIS->hwndItem, &r);
	POINT pt;
	pt.x = r.left;
	pt.y = r.top;
	::ScreenToClient(GetParent(lpDIS->hwndItem), &pt);

	if(lpDIS->CtlType == ODT_STATIC)
	{
		if(pBKImage)
		{
			// 画背景图
			Graphics *graphics = Graphics::FromHDC(hdc);
			if(graphics)
			{
				Bitmap bmp(width, height);
				Graphics * buffergraphics = Graphics::FromImage(&bmp);	//创建一个内存图像
				if(buffergraphics)
				{
					buffergraphics->DrawImage(pBKImage, RectF(0, 0, width, height), pt.x, pt.y, width, height, UnitPixel);
					graphics->DrawImage(&bmp, 0, 0, width, height);		//将内存图像显示到屏幕
					delete buffergraphics ; 
				}
				graphics->ReleaseHDC(hdc);
				delete graphics;
			}
		}
		else
		{
			// 无背景图，则以父窗口背景色填充
			LOGBRUSH br;
			br.lbStyle = BS_SOLID;
			br.lbHatch = HS_CROSS;
			br.lbColor = ::GetBkColor(::GetDC(::GetParent(lpDIS->hwndItem)));
			HBRUSH hBrush = ::CreateBrushIndirect(&br);
			::FillRect(hdc, &lpDIS->rcItem, hBrush);
			::DeleteObject(hBrush);
		}

		/*
		{
		// 获取子图（背景图）
		RECT subRect;
		subRect.left = pt.x;
		subRect.top = pt.y;
		subRect.right = subRect.left + width;
		subRect.bottom = subRect.top + height;
		HBITMAP hMap = GetSubBitmap(hdc, g_DownloadDlgBKMap, subRect);

		// 画背景图
		HDC hMemDC = ::CreateCompatibleDC(hdc);
		::SelectObject(hMemDC, hMap);
		::BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);
		::DeleteDC(hMemDC);
		}
		*/


		// font
		HFONT hFont = ::CreateFontIndirect(&g_font_text_default);
		if(hFont)
		{
			::SelectObject(hdc, hFont);
		}
		else
		{
			LOGFONT lf;
			::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
			hFont = ::CreateFontIndirect(&lf);
			::SelectObject(hdc, hFont);
			::DeleteObject(::GetStockObject(DEFAULT_GUI_FONT));
		}
		::DeleteObject(hFont);
		
		// color
		::SetTextColor(hdc, COLOR_TEXT_DEFAULT);

		// draw text
		::SetBkMode(hdc, TRANSPARENT);
		TCHAR lpString[255] = {0};
		::GetWindowText(lpDIS->hwndItem, lpString, sizeof(lpString) / sizeof(lpString[0]));
		::DrawText(hdc, lpString, wcslen(lpString), &lpDIS->rcItem, DT_LEFT | DT_TOP | DT_WORDBREAK);
	}
}
//void DrawButton(int nPage, Image *pBKImage, LPDRAWITEMSTRUCT lpDIS)
//{
//	HDC hdc = lpDIS->hDC;
//
//	// 控件的宽和高
//	int width = lpDIS->rcItem.right - lpDIS->rcItem.left;
//	int height = lpDIS->rcItem.bottom - lpDIS->rcItem.top;
//
//	// 获取该控件在父窗口中的坐标
//	RECT r;
//	::GetWindowRect(lpDIS->hwndItem, &r);
//	POINT pt;
//	pt.x = r.left;
//	pt.y = r.top;
//	::ScreenToClient(GetParent(lpDIS->hwndItem), &pt);
//
//	if(lpDIS->CtlType == ODT_BUTTON)
//	{
//		Graphics *graphics = Graphics::FromHDC(hdc);
//		if(graphics)
//		{
//			Bitmap bmp(width, height);
//			Graphics * buffergraphics = Graphics::FromImage(&bmp);	//创建一个内存图像
//			if(buffergraphics)
//			{
//				// draw bk first
//				if(pBKImage)
//				{
//					buffergraphics->DrawImage(pBKImage, RectF(0, 0, width, height), pt.x, pt.y, width, height, UnitPixel);
//				}
//
//				// draw btn map
//				Image *pImg = NULL;
//				UINT state = lpDIS->itemState;
//				if ((state & ODS_SELECTED) || (state & ODS_FOCUS))
//				{
//					pImg = g_pBtnImage_Select->Clone();
//				}
//				else if (state & ODS_DISABLED)
//				{
//					pImg = g_pBtnImage_Disable->Clone();
//				}
//				else
//				{
//					if(nPage == 1)
//					{
//						if(g_Main_btn_browse_bMouseOverButton || g_Main_btn_download_bMouseOverButton || g_Main_btn_cancel_bMouseOverButton)
//						{
//							// hover
//							pImg = g_pBtnImage_Hover->Clone();
//						}
//						else
//						{
//							// normal
//							pImg = g_pBtnImage_Normal->Clone();
//						}
//					}
//					else
//					{
//						if(g_DownLoad_btn_min_bMouseOverButton || g_DownLoad_btn_cancel_bMouseOverButton)
//						{
//							// hover
//							pImg = g_pBtnImage_Hover->Clone();
//						}
//						else
//						{
//							// normal
//							pImg = g_pBtnImage_Normal->Clone();
//						}
//					}
//				}
//				int nImgWidth  = pImg->GetWidth();
//				int nImgHeight = pImg->GetHeight();
//				buffergraphics->DrawImage(pImg, RectF(0, 0, width, height), 0, 0, nImgWidth, nImgHeight, UnitPixel);
//
//				/*
//				// draw btn map in one map
//				int nImgWidth  = g_pBtnImage->GetWidth();
//				int nImgHeight = g_pBtnImage->GetHeight();
//				UINT state = lpDIS->itemState;
//				if ((state & ODS_SELECTED) || (state & ODS_FOCUS))
//				{
//					buffergraphics->DrawImage(g_pBtnImage, RectF(0, 0, width, height), 0, nImgHeight / 4, nImgWidth, nImgHeight / 4, UnitPixel);
//				}
//				else if (state & ODS_DISABLED)
//				{
//					buffergraphics->DrawImage(g_pBtnImage, RectF(0, 0, width, height), 0, (3 * nImgHeight) / 4, nImgWidth, nImgHeight / 4, UnitPixel);
//				}
//				else
//				{
//					if(nPage == 1)
//					{
//						if(g_Main_btn_browse_bMouseOverButton || g_Main_btn_download_bMouseOverButton || g_Main_btn_cancel_bMouseOverButton)
//						{
//							// hover
//							buffergraphics->DrawImage(g_pBtnImage, RectF(0, 0, width, height), 0, (2 * nImgHeight) / 4, nImgWidth, nImgHeight / 4, UnitPixel);
//						}
//						else
//						{
//							// normal
//							buffergraphics->DrawImage(g_pBtnImage, RectF(0, 0, width, height), 0, 0, nImgWidth, nImgHeight / 4, UnitPixel);
//						}
//					}
//					else
//					{
//						if(g_DownLoad_btn_min_bMouseOverButton || g_DownLoad_btn_cancel_bMouseOverButton)
//						{
//							// hover
//							buffergraphics->DrawImage(g_pBtnImage, RectF(0, 0, width, height), 0, (2 * nImgHeight) / 4, nImgWidth, nImgHeight / 4, UnitPixel);
//						}
//						else
//						{
//							// normal
//							buffergraphics->DrawImage(g_pBtnImage, RectF(0, 0, width, height), 0, 0, nImgWidth, nImgHeight / 4, UnitPixel);
//						}
//					}
//				}
//				*/
//				graphics->DrawImage(&bmp, 0, 0, width, height);		//将内存图像显示到屏幕
//				delete buffergraphics ; 
//			}
//			graphics->ReleaseHDC(hdc);
//			delete graphics;
//		}
//
//		// font
//		HFONT hFont = NULL;
//		if(nPage == 2 && lpDIS->hwndItem == GetDlgItem(GetParent(lpDIS->hwndItem), IDC_INSTALL))
//		{
//			hFont = ::CreateFontIndirect(&g_font_install_btn_default);
//		}
//		else
//		{
//			hFont = ::CreateFontIndirect(&g_font_btn_default);
//		}
//		
//		if(hFont)
//		{
//			::SelectObject(hdc, hFont);
//		}
//		else
//		{
//			LOGFONT lf;
//			::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
//			hFont = ::CreateFontIndirect(&lf);
//			::SelectObject(hdc, hFont);
//			::DeleteObject(::GetStockObject(DEFAULT_GUI_FONT));
//		}
//		::DeleteObject(hFont);
//
//		// color
//		::SetTextColor(hdc, COLOR_TEXT_DEFAULT);
//
//		// draw text
//		::SetBkMode(hdc, TRANSPARENT);
//		TCHAR lpString[255] = {0};
//		::GetWindowText(lpDIS->hwndItem, lpString, sizeof(lpString) / sizeof(lpString[0]));
//		::DrawText(hdc, lpString, wcslen(lpString), &lpDIS->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
//	}
//}

//LRESULT CALLBACK DownLoad_Btn_Min_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	case WM_MOUSEMOVE:
//		{
//			if(!g_DownLoad_btn_min_bMouseOverButton)
//			{
//				TRACKMOUSEEVENT csTME;
//				csTME.cbSize = sizeof(csTME);
//				csTME.dwFlags = TME_LEAVE;
//				csTME.hwndTrack = hWnd;
//				::_TrackMouseEvent(&csTME);
//
//				g_DownLoad_btn_min_bMouseOverButton = TRUE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			}
//			return 0;
//		}
//		break;
//	case WM_MOUSELEAVE:
//		{
//			if (g_DownLoad_btn_min_bMouseOverButton)
//			{
//				g_DownLoad_btn_min_bMouseOverButton = FALSE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			} 
//			return 0;
//		}
//	case WM_SETCURSOR:
//		{
//			HCURSOR m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND);
//			if (m_hLinkCursor)
//			{
//				::SetCursor(m_hLinkCursor);
//				return TRUE;
//			}
//			break;
//		}
//	default:
//		break;
//	}
//	// Any messages we don't process must be passed onto the original window function
//	return CallWindowProc((WNDPROC)g_DownLoad_btn_min_proc, hWnd, message, wParam, lParam);
//}
//
//LRESULT CALLBACK DownLoad_Btn_Cancel_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	case WM_MOUSEMOVE:
//		{
//			if(!g_DownLoad_btn_cancel_bMouseOverButton)
//			{
//				TRACKMOUSEEVENT csTME;
//				csTME.cbSize = sizeof(csTME);
//				csTME.dwFlags = TME_LEAVE;
//				csTME.hwndTrack = hWnd;
//				::_TrackMouseEvent(&csTME);
//
//				g_DownLoad_btn_cancel_bMouseOverButton = TRUE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			}
//			return 0;
//		}
//		break;
//	case WM_MOUSELEAVE:
//		{
//			if (g_DownLoad_btn_cancel_bMouseOverButton)
//			{
//				g_DownLoad_btn_cancel_bMouseOverButton = FALSE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			} 
//			return 0;
//		}
//	case WM_SETCURSOR:
//		{
//			HCURSOR m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND);
//			if (m_hLinkCursor)
//			{
//				::SetCursor(m_hLinkCursor);
//				return TRUE;
//			}
//			break;
//		}
//	default:
//		break;
//	}
//	// Any messages we don't process must be passed onto the original window function
//	return CallWindowProc((WNDPROC)g_DownLoad_btn_cancel_proc, hWnd, message, wParam, lParam);
//}
//
//LRESULT CALLBACK Main_Btn_Browse_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	case WM_MOUSEMOVE:
//		{
//			if(!g_Main_btn_browse_bMouseOverButton)
//			{
//				TRACKMOUSEEVENT csTME;
//				csTME.cbSize = sizeof(csTME);
//				csTME.dwFlags = TME_LEAVE;
//				csTME.hwndTrack = hWnd;
//				::_TrackMouseEvent(&csTME);
//
//
//				g_Main_btn_browse_bMouseOverButton = TRUE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			}
//			return 0;
//		}
//		break;
//	case WM_MOUSELEAVE:
//		{
//			if (g_Main_btn_browse_bMouseOverButton)
//			{
//				g_Main_btn_browse_bMouseOverButton = FALSE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			} 
//			return 0;
//		}
//	case WM_SETCURSOR:
//		{
//			HCURSOR m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND);
//			if (m_hLinkCursor)
//			{
//				::SetCursor(m_hLinkCursor);
//				return TRUE;
//			}
//			break;
//		}
//	default:
//		break;
//	}
//	// Any messages we don't process must be passed onto the original window function
//	return CallWindowProc((WNDPROC)g_Main_btn_browse_proc, hWnd, message, wParam, lParam);
//}

//LRESULT CALLBACK DownLoad_Btn_Install_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	case WM_MOUSEMOVE:
//		{
//			if(!g_DownLoad_btn_install_bMouseOverButton)
//			{
//				TRACKMOUSEEVENT csTME;
//				csTME.cbSize = sizeof(csTME);
//				csTME.dwFlags = TME_LEAVE;
//				csTME.hwndTrack = hWnd;
//				::_TrackMouseEvent(&csTME);
//
//				g_DownLoad_btn_install_bMouseOverButton = TRUE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			}
//			return 0;
//		}
//		break;
//	case WM_MOUSELEAVE:
//		{
//			if (g_DownLoad_btn_install_bMouseOverButton)
//			{
//				g_DownLoad_btn_install_bMouseOverButton = FALSE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			} 
//			return 0;
//		}
//	case WM_SETCURSOR:
//		{
//			HCURSOR m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND);
//			if (m_hLinkCursor)
//			{
//				::SetCursor(m_hLinkCursor);
//				return TRUE;
//			}
//			break;
//		}
//	default:
//		break;
//	}
//	// Any messages we don't process must be passed onto the original window function
//	return CallWindowProc((WNDPROC)g_DownLoad_btn_install_proc, hWnd, message, wParam, lParam);
//}
//
//LRESULT CALLBACK Main_Btn_Download_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	case WM_MOUSEMOVE:
//		{
//			if(!g_Main_btn_download_bMouseOverButton)
//			{
//				TRACKMOUSEEVENT csTME;
//				csTME.cbSize = sizeof(csTME);
//				csTME.dwFlags = TME_LEAVE;
//				csTME.hwndTrack = hWnd;
//				::_TrackMouseEvent(&csTME);
//
//				g_Main_btn_download_bMouseOverButton = TRUE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			}
//			return 0;
//		}
//		break;
//	case WM_MOUSELEAVE:
//		{
//			if (g_Main_btn_download_bMouseOverButton)
//			{
//				g_Main_btn_download_bMouseOverButton = FALSE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			} 
//			return 0;
//		}
//	case WM_SETCURSOR:
//		{
//			HCURSOR m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND);
//			if (m_hLinkCursor)
//			{
//				::SetCursor(m_hLinkCursor);
//				return TRUE;
//			}
//			break;
//		}
//	default:
//		break;
//	}
//	// Any messages we don't process must be passed onto the original window function
//	return CallWindowProc((WNDPROC)g_Main_btn_download_proc, hWnd, message, wParam, lParam);
//}
//
//LRESULT CALLBACK Main_Btn_Cancel_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	case WM_MOUSEMOVE:
//		{
//			if(!g_Main_btn_cancel_bMouseOverButton)
//			{
//				TRACKMOUSEEVENT csTME;
//				csTME.cbSize = sizeof(csTME);
//				csTME.dwFlags = TME_LEAVE;
//				csTME.hwndTrack = hWnd;
//				::_TrackMouseEvent(&csTME);
//
//				g_Main_btn_cancel_bMouseOverButton = TRUE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			}
//			return 0;
//		}
//		break;
//	case WM_MOUSELEAVE:
//		{
//			if (g_Main_btn_cancel_bMouseOverButton)
//			{
//				g_Main_btn_cancel_bMouseOverButton = FALSE;
//				InvalidateRect(hWnd, NULL, FALSE);
//			} 
//			return 0;
//		}
//	case WM_SETCURSOR:
//		{
//			HCURSOR m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND);
//			if (m_hLinkCursor)
//			{
//				::SetCursor(m_hLinkCursor);
//				return TRUE;
//			}
//			break;
//		}
//	default:
//		break;
//	}
//	// Any messages we don't process must be passed onto the original window function
//	return CallWindowProc((WNDPROC)g_Main_btn_cancel_proc, hWnd, message, wParam, lParam);
//}

CString MyGetDefaultFolder()
{
	std::wstring folder = GetDefaultFolder();
	CString szDefaultFolder = folder.c_str();
	return szDefaultFolder;
}

BOOL ReadConfig(CString szConfigFilePathName, CString &szSavePath, CString &szSaveName, CString &szBoxSaveName, BOOL &bComplete)
{
	if(szConfigFilePathName.IsEmpty())
	{
		return FALSE;
	}

	BOOL bRet = FALSE;
	CMarkup xml;
	std::wstring strPath;
	std::wstring strName;
	std::wstring strComplete;

	if(xml.Load(szConfigFilePathName.GetBuffer(szConfigFilePathName.GetLength())))
	{
		if(xml.FindElem(_T("config")))
		{
			if(xml.FindChildElem(_T("task")))
			{
				xml.IntoElem();
				if(xml.FindChildElem(_T("path")))
				{
					xml.IntoElem();
					//strPath = xml.GetElemContent();
					strPath = xml.GetData();
					if('\\' != strPath.at(strPath.length() - 1))
					{
						strPath += _T("\\");
					}
					szSavePath.Format(_T("%s"), strPath.c_str());
					xml.OutOfElem();
					bRet = TRUE;
				}
				else
				{
					bRet = FALSE;
				}

				if(bRet)
				{
					if(xml.FindChildElem(_T("filename")))
					{
						xml.IntoElem();
						//strName = xml.GetElemContent();
						strName = xml.GetData();
						xml.OutOfElem();

						szSaveName.Format(_T("%s"), strName.c_str());
						bRet = TRUE;
					}
					else
					{
						bRet = FALSE;
					}
				}

				if(bRet)
				{
					if(xml.FindChildElem(_T("boxfilename")))
					{
						xml.IntoElem();
						//strName = xml.GetElemContent();
						strName = xml.GetData();
						xml.OutOfElem();

						szBoxSaveName.Format(_T("%s"), strName.c_str());
						bRet = TRUE;
					}
					else
					{
						bRet = FALSE;
					}
				}

				if(bRet)
				{
					if(xml.FindChildElem(_T("complete")))
					{
						xml.IntoElem();
						//strComplete = xml.GetElemContent();
						strComplete = xml.GetData();
						xml.OutOfElem();

						if(0 == strComplete.compare(_T("yes")))
						{
							bComplete = TRUE;
						}
						else
						{
							bComplete = FALSE;
						}
						bRet = TRUE;
					}
					else
					{
						bRet = FALSE;
					}
				}

				xml.OutOfElem();
			}
		}
	}
	return bRet;
}

BOOL SaveConfig(CString szConfigFilePathName, CString szSavePath, CString szSaveName, CString szBoxSaveName, BOOL bComplete)
{
	if(szConfigFilePathName.IsEmpty())
	{
		return FALSE;
	}

	BOOL bRet = FALSE;
	CMarkup xml;
	xml.SetDoc(_T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"));
	if(xml.AddElem(_T("config")))
	{
		xml.IntoElem();
		if(xml.AddElem(_T("task")))
		{
			if(xml.AddChildElem( _T("path"), szSavePath.GetBuffer(szSavePath.GetLength())))
			{
				bRet = TRUE;
			}
			if(bRet)
			{
				if(xml.AddChildElem( _T("filename"), szSaveName.GetBuffer(szSaveName.GetLength())))
				{
					bRet = TRUE;
				}
				else
				{
					bRet = FALSE;
				}
			}
			if(bRet)
			{
				if(xml.AddChildElem( _T("boxfilename"), szBoxSaveName.GetBuffer(szBoxSaveName.GetLength())))
				{
					bRet = TRUE;
				}
				else
				{
					bRet = FALSE;
				}
			}
			if(bRet)
			{
				CString szComplete = _T("no");
				if(bComplete)
				{
					szComplete = _T("yes");
				}
				if(xml.AddChildElem( _T("complete"), szComplete.GetBuffer(szComplete.GetLength())))
				{
					bRet = TRUE;
				}
				else
				{
					bRet = FALSE;
				}
			}
		}
		xml.OutOfElem();
	}
	xml.Save(szConfigFilePathName.GetBuffer(szConfigFilePathName.GetLength()));
	return bRet;
}

////////////////////////////////////////////////////////////////////////////
///****************************************************************************
//*功能：生成任务栏托盘图标
//*参数：
//*	1：hWnd － 窗口句柄
//*返回：
//*	
//*作者：tyc:2008-08-01
//*注释：
//*****************************************************************************/
//BOOL CreateTray(HWND hWnd, TCHAR *pTip)
//{
//	NOTIFYICONDATA tnd;
//	tnd.cbSize = sizeof(NOTIFYICONDATA);
//	tnd.hWnd = hWnd;
//	tnd.uID = IDI_LIVEBOOTDOWNLOADER;
//	tnd.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
//	tnd.uCallbackMessage = WM_TRAY_NOTIFYCATION;
//	tnd.hIcon = ::LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_LIVEBOOTDOWNLOADER));
//
//	TCHAR info[50] = {0};
//	swprintf(info, _T("%s"), pTip);
//
//	wcscpy_s(tnd.szTip, sizeof(tnd.szTip) ,info);
//	return Shell_NotifyIcon(NIM_ADD, &tnd);
//}
//
//
///****************************************************************************
//*功能：删除任务栏托盘图标
//*参数：
//*	1：hWnd －窗口句柄
//*返回：
//*	
//*作者：tyc:2008-08-01
//*注释：
//*****************************************************************************/
//BOOL DeleteTray(HWND hWnd)
//{
//	NOTIFYICONDATA tnid;
//	tnid.cbSize = sizeof(NOTIFYICONDATA);
//	tnid.hWnd = hWnd;
//	tnid.uID = IDI_LIVEBOOTDOWNLOADER;
//	return Shell_NotifyIcon(NIM_DELETE, &tnid);
//}
//
//LRESULT OnTrayProc(HWND hDlg, WPARAM wParam, LPARAM lParam)
//{
//	UINT uID = (UINT)wParam;		//发出该消息的图标的ID
//	UINT uMouseMsg = (UINT)lParam;	//鼠标动作
//	switch(uMouseMsg)
//	{
//	case WM_RBUTTONUP:
//	case WM_LBUTTONUP:
//	case WM_LBUTTONDBLCLK:
//	case WM_RBUTTONDBLCLK:
//		{
//			if(uID == IDI_LIVEBOOTDOWNLOADER)
//			{
//				::ShowWindow(g_hMainWnd, SW_SHOWNORMAL);
//			}
//			break;
//		}
//	default:
//		break;
//	}
//	return true;
//}
//
///****************************************************************************
//*功能：设置托盘提示信息
//*参数：
//*	1：hWnd      －窗口句柄
//*   2：szTitle   - 提示标题
//*   3：szTip     - 提示信息
//*返回：
//*	
//*作者：tyc:2008-08-01
//*注释：
//*****************************************************************************/
//BOOL SetTrayBalloonTip(HWND hWnd , TCHAR *szTitle, TCHAR *szTip, UINT nTimeout)
//{
//	NOTIFYICONDATA m_nid;
//	m_nid.cbSize = sizeof(NOTIFYICONDATA);		
//	m_nid.hWnd = hWnd;
//	m_nid.uID = IDI_LIVEBOOTDOWNLOADER;
//	m_nid.uFlags = NIF_INFO;
//	m_nid.uTimeout = nTimeout;
//	m_nid.dwInfoFlags = NIIF_INFO;
//	memset(m_nid.szInfoTitle, 0, sizeof(m_nid.szInfoTitle));
//	memset(m_nid.szInfo, 0, sizeof(m_nid.szInfo));	
//
//	memcpy_s(m_nid.szInfoTitle, sizeof(m_nid.szInfoTitle) ,szTitle ,sizeof(m_nid.szInfoTitle));
//	memcpy_s(m_nid.szInfo, sizeof(m_nid.szInfo), szTip, sizeof(m_nid.szInfo));
//	return Shell_NotifyIcon(NIM_MODIFY, &m_nid);
//}

//void DrawMainDlgBk(HWND hwnd, HDC hdc, Image *pImage)
//{
//	RECT rt;
//	GetClientRect(hwnd, &rt);
//	int width = rt.right - rt.left;
//	int height = rt.bottom - rt.top;
//
//	if(pImage)
//	{
//		Graphics *graphics = Graphics::FromHDC(hdc);
//		if(graphics)
//		{
//			Bitmap bmp(width, height);
//			Graphics * buffergraphics = Graphics::FromImage(&bmp);	//关键部分，创建一个内存图像
//			if(buffergraphics)
//			{
//				buffergraphics->DrawImage(pImage, RectF(0, 0, width, height));
//				graphics->DrawImage(&bmp, 0, 0, width, height);		//将内存图像显示到屏幕
//				delete buffergraphics ; 
//			}
//			graphics->ReleaseHDC(hdc);
//			delete graphics;
//		}
//	}
//
//	// font
//	HFONT hFont = ::CreateFontIndirect(&g_font_text_default);
//	if(hFont)
//	{
//		::SelectObject(hdc, hFont);
//	}
//	else
//	{
//		LOGFONT lf;
//		::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
//		hFont = ::CreateFontIndirect(&lf);
//		::SelectObject(hdc, hFont);
//		::DeleteObject(::GetStockObject(DEFAULT_GUI_FONT));
//	}
//	::DeleteObject(hFont);
//
//	// color
//	::SetTextColor(hdc, COLOR_TEXT_DEFAULT);
//
//	::SetBkMode(hdc, TRANSPARENT);
//	RECT rc;
//	CString szText;
//
//	//GdiRectToRECT(g_rc_MainDlg_static_savepath, rc);
//	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_MainDlg_static_savepath);
//	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
//
//	GdiRectToRECT(g_rc_MainDlg_static_savepath, rc);
//	szText = StringFromIDResource(g_hInst, ID_TEXT_lang_MainDlg_static_savepath);
//	::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
//}

void DrawDownLoadDlgBk(HWND hwnd, HDC hdc, Image *pImage)
{
	RECT rt;
	GetClientRect(hwnd, &rt);
	int width = rt.right - rt.left;
	int height = rt.bottom - rt.top;

	if(pImage)
	{
		Graphics *graphics = Graphics::FromHDC(hdc);
		if(graphics)
		{
			Bitmap bmp(width, height);
			Graphics * buffergraphics = Graphics::FromImage(&bmp);	//关键部分，创建一个内存图像
			if(buffergraphics)
			{
				buffergraphics->DrawImage(pImage, RectF(0, 0, width, height));
				graphics->DrawImage(&bmp, 0, 0, width, height);		//将内存图像显示到屏幕
				delete buffergraphics ; 
			}
			graphics->ReleaseHDC(hdc);
			delete graphics;
		}
	}

	// font
	HFONT hFont = ::CreateFontIndirect(&g_font_text_default);
	if(hFont)
	{
		::SelectObject(hdc, hFont);
	}
	else
	{
		LOGFONT lf;
		::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
		hFont = ::CreateFontIndirect(&lf);
		::SelectObject(hdc, hFont);
		::DeleteObject(::GetStockObject(DEFAULT_GUI_FONT));
	}
	::DeleteObject(hFont);

	// color
	::SetTextColor(hdc, COLOR_TEXT_DEFAULT);

	::SetBkMode(hdc, TRANSPARENT);
	//RECT rc;
	//GdiRectToRECT(g_rc_DownLoadDlg_text, rc);
	//CString szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_text);
	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);

	//GdiRectToRECT(g_rc_DownLoadDlg_static_speed, rc);
	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_static_speed);
	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);

	//GdiRectToRECT(g_rc_DownLoadDlg_static_lefttime, rc);
	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_static_lefttime);
	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);

	//GdiRectToRECT(g_rc_DownLoadDlg_static_filesize, rc);
	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_static_filesize);
	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);

	//GdiRectToRECT(g_rc_DownLoadDlg_static_remainingsize, rc);
	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_static_downloadedsize);
	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
}

//void DrawDefaultBKImage(HWND hwnd, HDC hdc, Image *pImage)
//{
//	RECT rt;
//	GetClientRect(hwnd, &rt);
//	int width = rt.right - rt.left;
//	int height = rt.bottom - rt.top - 140;
//
//	if(pImage)
//	{
//		Graphics *graphics = Graphics::FromHDC(hdc);
//		if(graphics)
//		{
//			Bitmap bmp(width, height);
//			Graphics * buffergraphics = Graphics::FromImage(&bmp);	//关键部分，创建一个内存图像
//			if(buffergraphics)
//			{
//				buffergraphics->DrawImage(pImage, RectF(0, 0, width, height));
//				graphics->DrawImage(&bmp, 0, 0, width, height);		//将内存图像显示到屏幕
//				delete buffergraphics ; 
//			}
//			graphics->ReleaseHDC(hdc);
//			delete graphics;
//		}
//	}
//
//	// font
//	HFONT hFont = ::CreateFontIndirect(&g_font_text_default);
//	if(hFont)
//	{
//		::SelectObject(hdc, hFont);
//	}
//	else
//	{
//		LOGFONT lf;
//		::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
//		hFont = ::CreateFontIndirect(&lf);
//		::SelectObject(hdc, hFont);
//		::DeleteObject(::GetStockObject(DEFAULT_GUI_FONT));
//	}
//	::DeleteObject(hFont);
//
//	// color
//	::SetTextColor(hdc, COLOR_TEXT_DEFAULT);
//
//	::SetBkMode(hdc, TRANSPARENT);
//	//RECT rc;
//	//GdiRectToRECT(g_rc_DownLoadDlg_text, rc);
//	//CString szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_text);
//	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
//
//	//GdiRectToRECT(g_rc_DownLoadDlg_static_speed, rc);
//	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_static_speed);
//	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
//
//	//GdiRectToRECT(g_rc_DownLoadDlg_static_lefttime, rc);
//	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_static_lefttime);
//	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
//
//	//GdiRectToRECT(g_rc_DownLoadDlg_static_filesize, rc);
//	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_static_filesize);
//	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
//
//	//GdiRectToRECT(g_rc_DownLoadDlg_static_remainingsize, rc);
//	//szText = StringFromIDResource(g_hInst, ID_TEXT_lang_DownLoadDlg_static_downloadedsize);
//	//::DrawText(hdc, szText, szText.GetLength(), &rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
//}

BOOL InitRes(HINSTANCE hInst)
{
	BOOL bRet = FALSE;

	// Init default font
	{
		TCHAR *pFaceName = _T("Tahoma");

		g_font_btn_default.lfHeight = GetFontHeight(FONT_BUTTON_DEFAULT_SIZE);
		g_font_btn_default.lfWidth = 0;
		g_font_btn_default.lfEscapement = 0;
		g_font_btn_default.lfOrientation = 0;
		g_font_btn_default.lfWeight = FW_NORMAL;
		g_font_btn_default.lfItalic = FALSE;
		g_font_btn_default.lfUnderline = FALSE;
		g_font_btn_default.lfStrikeOut = 0;
		g_font_btn_default.lfCharSet = ANSI_CHARSET;
		g_font_btn_default.lfOutPrecision = OUT_DEFAULT_PRECIS;
		g_font_btn_default.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		g_font_btn_default.lfQuality = DEFAULT_QUALITY;
		g_font_btn_default.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
		memcpy(g_font_btn_default.lfFaceName, pFaceName, wcslen(pFaceName));

		g_font_text_default.lfHeight = GetFontHeight(FONT_TEXT_DEFAULT_SIZE);
		g_font_text_default.lfWidth = 0;
		g_font_text_default.lfEscapement = 0;
		g_font_text_default.lfOrientation = 0;
		g_font_text_default.lfWeight = FW_NORMAL;
		g_font_text_default.lfItalic = FALSE;
		g_font_text_default.lfUnderline = FALSE;
		g_font_text_default.lfStrikeOut = 0;
		g_font_text_default.lfCharSet = ANSI_CHARSET;
		g_font_text_default.lfOutPrecision = OUT_DEFAULT_PRECIS;
		g_font_text_default.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		g_font_text_default.lfQuality = DEFAULT_QUALITY;
		g_font_text_default.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
		memcpy(g_font_text_default.lfFaceName, pFaceName, wcslen(pFaceName));

		g_font_install_btn_default.lfHeight = GetFontHeight(FONT_INSTALL_BUTTON_DEFAULT_SIZE);
		g_font_install_btn_default.lfWidth = 0;
		g_font_install_btn_default.lfEscapement = 0;
		g_font_install_btn_default.lfOrientation = 0;
		g_font_install_btn_default.lfWeight = FW_MEDIUM;
		g_font_install_btn_default.lfItalic = FALSE;
		g_font_install_btn_default.lfUnderline = FALSE;
		g_font_install_btn_default.lfStrikeOut = 0;
		g_font_install_btn_default.lfCharSet = ANSI_CHARSET;
		g_font_install_btn_default.lfOutPrecision = OUT_DEFAULT_PRECIS;
		g_font_install_btn_default.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		g_font_install_btn_default.lfQuality = DEFAULT_QUALITY;
		g_font_install_btn_default.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
		memcpy(g_font_install_btn_default.lfFaceName, pFaceName, wcslen(pFaceName));
	}

	// Load bitmap
	{
		RECT r;
		::GetClientRect(g_hMainWnd, &r);
		//g_pBtnImage_Normal = ImageFromIDResource(hInst, IDB_PNG_BTN_NORMAL, _T("PNG"));
		//g_pBtnImage_Hover = ImageFromIDResource(hInst, IDB_PNG_BTN_HOVER, _T("PNG"));
		//g_pBtnImage_Select = ImageFromIDResource(hInst, IDB_PNG_BTN_SELECT, _T("PNG"));
		//g_pBtnImage_Disable= ImageFromIDResource(hInst, IDB_PNG_BTN_DISABLE, _T("PNG"));
		Image* pOldBkImage = ImageFromIDResource(hInst, IDB_PNG_BK, _T("PNG"));
		g_pDownloadDlgBKImage = GetZoomImage(pOldBkImage, r.right - r.left, r.bottom - r.top);

		Image* pEmptyOldBkImage = ImageFromIDResource(hInst, IDB_PNG_BK_EMPTY, _T("PNG"));
		g_pEmptyDownloadDlgBKImage = GetZoomImage(pEmptyOldBkImage, r.right - r.left, r.bottom - r.top);

		//Image* pOldBkImage2 = ImageFromIDResource(hInst, IDB_PNG_DEFAULTBK, _T("PNG"));
		//g_pDefaultBKImage = GetZoomImage(pOldBkImage2, r.right - r.left, r.bottom - r.top - 140);

		//if(g_pBtnImage_Normal && g_pBtnImage_Hover 
		//	&& g_pBtnImage_Select && g_pBtnImage_Disable 
		//	&& g_pDownloadDlgBKImage
		//	&& g_pEmptyDownloadDlgBKImage/*&& g_pDefaultBKImage*/)
		if(g_pDownloadDlgBKImage && g_pEmptyDownloadDlgBKImage)
		{
			bRet = TRUE;
		}
	}

	return bRet;
}

BOOL GetDownloadConfigInfoFromResource(HINSTANCE hInst)
{
	//////////////////////////////////////////////////////////////////////////
	//其余变量改由服务器给出
	g_Default_SaveFileName.Format(_T("inst_%d.exe"), g_ProductPID);
	g_wac_saveFileName.Format(_T("inst_%d.exe"), g_WacPID);
	return TRUE;

	//g_DownloadURL = StringFromIDResource(hInst, ID_Download_URL);
	//g_MD5 = StringFromIDResource(hInst, ID_Download_MD5);
	//g_MD5.MakeUpper();
	//g_Default_SaveFileName = StringFromIDResource(hInst, ID_Download_Default_SaveName);
	//if(g_DownloadURL.IsEmpty() || g_MD5.IsEmpty() || g_Default_SaveFileName.IsEmpty())
	//{
	//	return FALSE;
	//}

	//g_wac_url = StringFromIDResource(hInst, ID_WAC_URL);
	//g_wac_md5 = StringFromIDResource(hInst, ID_WAC_MD5);
	//g_wac_md5.MakeUpper();
	//g_wac_saveFileName = StringFromIDResource(hInst, ID_WAC_SAVEFILENAME);
	//if(g_wac_url.IsEmpty() || g_wac_md5.IsEmpty() || g_wac_saveFileName.IsEmpty())
	//{
	//	return FALSE;
	//}

	//return TRUE;
}

BOOL InitLog(CString szLogFilePathName)
{
#ifndef __LOG_ENABLE__
	return TRUE;
#endif

	BOOL bLog = FALSE;
	Logger *pLog = new Logger(szLogFilePathName.GetBuffer(szLogFilePathName.GetLength()));
	if(pLog)
	{
		if(pLog->IsOK())
		{
			bLog = TRUE;
			FileLogger::addAppender(LOG_UI_APPENDER_NAME, pLog);
		}
	}
	if(!bLog)
	{
		// ui log init failed
		if(pLog)
		{
			delete pLog;
			pLog = NULL;
		}
		return FALSE;
	}
	return TRUE;
}

void Clean()
{
	if(g_pFunctionInterface)
	{
		//g_pFunctionInterface->Clear();
		//delete g_pFunctionInterface;
		ns_WAC::DownloaderManagerFactory::release();
		g_pFunctionInterface = NULL;
	}
	FileLogger::removeAllAppenders();
}

BOOL Init(HINSTANCE hInst)
{
	//// log
	//if(!InitLog(GetExeRunDirectory(hInst) + LOG_UI_FILE_NAME))
	//{
	//	__LOG_LOG__( _T("log init failed."), L_LOGSECTION_APP_GENERAL, L_LOGLEVEL_ERROR);
	//	return FALSE;
	//}

	// register system message
	g_nMessageID = ::RegisterWindowMessage(_T("{02EC5224-1E2A-4aa0-95AF-8A7E528FF5F1}"));
	if(g_nMessageID == 0)
	{
		__LOG_LOG__( _T("register window message failed."), L_LOGSECTION_APP_GENERAL, L_LOGLEVEL_ERROR);
		return FALSE;
	}

	if(!InitRes(hInst))
	{
		__LOG_LOG__( _T("res init failed."), L_LOGSECTION_APP_GENERAL, L_LOGLEVEL_ERROR);
		return FALSE;
	}

	// create function interface
	g_pFunctionInterface = ns_WAC::DownloaderManagerFactory::get();
	if(!g_pFunctionInterface)
	{
		__LOG_LOG__( _T("function interface init failed."), L_LOGSECTION_APP_GENERAL, L_LOGLEVEL_ERROR);
		return FALSE;
	}
	return TRUE;
}

CString GetServerMd5(const CString& url)
{
	ChttpRequest req;
	CString str_md5;
	std::wstring str_url = url;
	BOOL rv = req.GetMD5(str_url.c_str(), str_md5);
	if(rv)
	{
		return str_md5;
	}
	else
	{
		return _T("");
	}
}
