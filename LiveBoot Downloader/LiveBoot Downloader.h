/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:22
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader\LiveBoot Downloader.h
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader
	file base:	LiveBoot Downloader
	file ext:	h
	author:		tanyc
	
	purpose:	
*********************************************************************/
#pragma once
#include "resource.h"
#include "CommCtrl.h"
#include "common.h"
#include <vector>
#include "../../Common/Markup.h"
#include "../dll_downloader/DownloaderManager.h"
#include "../dll_downloader/IDownloaderManager.h"
#include "PidInfo.h"
#include "task_track.h"

#ifdef _DEBUG
	#pragma comment(lib, "../../Debug/dll_downloader.lib")
#else
	#pragma comment(lib, "../../Release/dll_downloader.lib")
#endif


//////////////////////////////////////////////////////////////////////////
#define WM_TRAY_NOTIFYCATION		WM_USER + 170
//#define DOWNLOAD_TASK_XML_FILENAME	_T("DownloadTask.xml")
#define SINGLE_INSTANCE_CHECK_NAME	_T("{1DCC2C65-1109-45f8-AD2C-1CE27709ED3F}")
#define SINGLE_INSTANCE_BOX_CHECK_NAME	_T("{B97813A9-A2F9-4670-859E-EAA07F77B45C}")
#define DEFAULT_WINDOWS_TITLE	_T("Wondershare Software")

// 下载最大线程数
#define DOWNLOAD_MAX_THREAD_NUM		5

// 是否检查MD5(yes/no)
#define DOWNLOAD_CHECK_MD5			_T("yes")

// 窗口宽度(width border = 6)
#define WINODW_WIDTH				554 + 6
//#define WINODW_WIDTH				602 + 6

// 窗口高度(height border & title = 28)
#define WINDOW_HEIGHT				404 
//#define WINDOW_HEIGHT				369 + 28

#define FONT_BUTTON_DEFAULT_SIZE	12
#define FONT_INSTALL_BUTTON_DEFAULT_SIZE	20
#define FONT_TEXT_DEFAULT_SIZE		14
#define COLOR_TEXT_DEFAULT			RGB(0, 0, 0)


ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
BOOL Init(HINSTANCE hInst);
BOOL InitRes(HINSTANCE hInst);
BOOL InitLog(CString szLogFilePathName);
void Clean();
BOOL GetDownloadConfigInfoFromResource(HINSTANCE hInst);
//INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DownLoadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK Main_Btn_Browse_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK Main_Btn_Download_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK Main_Btn_Cancel_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK DownLoad_Btn_Min_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK DownLoad_Btn_Cancel_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK DownLoad_Btn_Install_Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void DrawStatic(Image *pBKImage, LPDRAWITEMSTRUCT lpDIS);
//void DrawButton(int nPage, Image *pBKImage, LPDRAWITEMSTRUCT lpDIS);
//void DrawMainDlgBk(HWND hwnd, HDC hdc, Image *pImage);
//void DrawDefaultBKImage(HWND hwnd, HDC hdc, Image *pImage);
void DrawDownLoadDlgBk(HWND hwnd, HDC hdc, Image *pImage);
CString MyGetDefaultFolder();
BOOL ReadConfig(CString szConfigFilePathName, CString &szSavePath, CString &szSaveName, CString &szBoxSaveName, BOOL &bComplete);
BOOL SaveConfig(CString szConfigFilePathName, CString szSavePath, CString szSaveName, CString szBoxSaveName, BOOL bComplete);
//BOOL CreateTray(HWND hWnd, TCHAR *pTip);
//BOOL DeleteTray(HWND hWnd);
//LRESULT OnTrayProc(HWND hDlg,WPARAM wParam, LPARAM lParam);
//BOOL SetTrayBalloonTip(HWND hWnd , TCHAR *szTitle, TCHAR *szTip, UINT nTimeout = 2000);
CString GetServerMd5(const CString& url);
std::wstring CurrentFormatTime();


UINT				g_nMessageID = 0; //用于设置回调函数
HINSTANCE			g_hInst;		//程序句柄
HWND				g_hMainWnd = NULL;
TCHAR				g_szWindowClass[100] = _T("WACDOWNLOADER");
GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR           gdiplusToken;

CString				g_DownLoadSaveFolder; //保存路径

//////////////////////////////////////////////////////////////////////////
//产品
CString             g_DownloadURL; //产品下载url
CString             g_MD5;			//产品md5，可以为空,从服务器读取
CString             g_Default_SaveFileName;
CString             g_Real_SaveFileName;
int					g_download_task_id = -1; //下载任务的ID
int					g_ProductPID = -1;//由exe指定位置读取
BOOL				g_ProductDownSucc = -1;//-1,未知，0，失败，1成功
__int64				g_product_filesize = -1;

//////////////////////////////////////////////////////////////////////////
//BOX
CString				g_wac_url; //box 下载url
//CString				g_wac_md5; //box MD5,可以为空
CString				g_wac_saveFileName;
CString				g_wac_RealSaveFileName;
int					g_wac_task_id = -1; //下载任务的ID
int					g_WacPID = -1;//由服务器获取url，然后从url中提取
BOOL				g_wac_DownSucc = -1;//-1,未知，0，失败，1成功
__int64				g_wac_filesize = -1;

//是否需要下载box标志，当有其他下载进程在下载box的时候，本进程不下载box
int					g_need_download_box = TRUE;

//取代DOWNLOAD_TASK_XML_FILENAME
CString				g_downtask_xml_filename;
//////////////////////////////////////////////////////////////////////////
BOOL				g_IsDownloading = FALSE;
BOOL				g_IsNeverDownloaded = TRUE;
BOOL                g_IsDownloadSuccessful = FALSE;
ns_WAC::IDownloaderManager *g_pFunctionInterface = NULL;
Image				*g_pDownloadDlgBKImage = NULL;
Image				*g_pEmptyDownloadDlgBKImage = NULL;
//Image				*g_pBtnImage_Normal = NULL;
//Image				*g_pBtnImage_Hover = NULL;
//Image				*g_pBtnImage_Select = NULL;
//Image				*g_pBtnImage_Disable= NULL;

//Image				*g_pDefaultBKImage = NULL;
int					g_ShowUrlSeq = 0;

//网页控件
//CAxWindow g_MainDlgWinContainer;
//BOOL g_MainDlgWinCreated = FALSE;//标识是否有创建网页控件

CAxWindow g_DownDlgWinContainer;
BOOL g_DownDlgWinCreated = FALSE;//标识是否有创建网页控件
IWebBrowser2* g_iwebBrowser2 = NULL;

//保存由服务器返回的pid产品信息
CPIDInfos g_pid_info;

//统计信息
TaskTrack g_task_track;

//上报下载跟踪信息给服务器，只上报一次
BOOL g_already_report = FALSE;

//保存产品先完成的下载结果，防止产品先于桌面中心完成，且失败，丢失错误码
ns_WAC::NsResultEnum g_product_download_result = ns_WAC::RS_SUCCESS;

//是否直接进入安装页
BOOL g_into_install = FALSE;

//static LONG g_Main_btn_browse_proc;
//static LONG g_Main_btn_download_proc;
//static LONG g_Main_btn_cancel_proc;
//static LONG g_DownLoad_btn_min_proc;
//static LONG g_DownLoad_btn_cancel_proc;
//static LONG g_DownLoad_btn_install_proc;
//static BOOL g_Main_btn_browse_bMouseOverButton = FALSE;
//static BOOL g_Main_btn_download_bMouseOverButton = FALSE;
//static BOOL g_Main_btn_cancel_bMouseOverButton = FALSE;
//static BOOL g_DownLoad_btn_min_bMouseOverButton = FALSE;
//static BOOL g_DownLoad_btn_cancel_bMouseOverButton = FALSE;
//static BOOL g_DownLoad_btn_install_bMouseOverButton = FALSE;


// 首界面位置信息
//Rect g_rc_MainDlg_text(50, 130, 500, 300);//暂不管
//Rect g_rc_MainDlg_static_savepath(20, 298, 70, 32);
//Rect g_rc_MainDlg_edit_savepath(90, 298, 345, 20);
//Rect g_rc_MainDlg_btn_browse(455, 292, 71, 32);
//Rect g_rc_MainDlg_btn_download(375, 332, 71, 32);
//Rect g_rc_MainDlg_btn_cancel(455, 332, 71, 32);

// 下载界面位置信息
//Rect g_rc_DownLoadDlg_text(50, 130, 500, 300);//暂不管
//Rect g_rc_DownLoadDlg_static_speed(150, 220, 50, 30);//暂不管
//Rect g_rc_DownLoadDlg_static_speed_value(200, 220, 100, 30);//暂不管
//Rect g_rc_DownLoadDlg_static_lefttime(350, 220, 100, 30);//暂不管
//Rect g_rc_DownLoadDlg_static_lefttime_value(450, 220, 150, 30);//暂不管
Rect g_rc_DownLoadDlg_static_progress(500, 302, 100, 30);
//
//Rect g_rc_DownLoadDlg_static_filesize(150, 280, 70, 30);//暂不管
//Rect g_rc_DownLoadDlg_static_filesize_value(220, 280, 100, 30);//暂不管
//Rect g_rc_DownLoadDlg_static_remainingsize(350, 280, 80, 30);//暂不管
//Rect g_rc_DownLoadDlg_static_remainingsize_value(430, 280, 100, 30);//暂不管

//Rect g_rc_DownLoadDlg_btn_min(350, 331, 110, 32);
//Rect g_rc_DownLoadDlg_btn_cancel(469, 331, 71, 32);
Rect g_rc_DownLoadDlg_progress(30, 302, 450, 20);

//安装按钮的位置
Rect g_rc_DownLoadDlg_install(195, 292, 160, 50);


//// 首界面位置信息
//Rect g_rc_MainDlg_text(50, 130, 500, 300);
//Rect g_rc_MainDlg_static_savepath(50, 270, 80, 30);
//Rect g_rc_MainDlg_edit_savepath(130, 270, 345, 20);
//Rect g_rc_MainDlg_btn_browse(500, 270, 71, 32);
//Rect g_rc_MainDlg_btn_download(400, 320, 71, 32);
//Rect g_rc_MainDlg_btn_cancel(500, 320, 71, 32);
//
//// 下载界面位置信息
//Rect g_rc_DownLoadDlg_text(50, 130, 500, 300);
//Rect g_rc_DownLoadDlg_static_speed(150, 220, 50, 30);
//Rect g_rc_DownLoadDlg_static_speed_value(200, 220, 100, 30);
//Rect g_rc_DownLoadDlg_static_lefttime(350, 220, 100, 30);
//Rect g_rc_DownLoadDlg_static_lefttime_value(450, 220, 150, 30);
//Rect g_rc_DownLoadDlg_static_progress(520, 250, 100, 30);
//
//Rect g_rc_DownLoadDlg_static_filesize(150, 280, 70, 30);
//Rect g_rc_DownLoadDlg_static_filesize_value(220, 280, 100, 30);
//Rect g_rc_DownLoadDlg_static_remainingsize(350, 280, 80, 30);
//Rect g_rc_DownLoadDlg_static_remainingsize_value(430, 280, 100, 30);
//
//Rect g_rc_DownLoadDlg_btn_min(375, 320, 110, 32);
//Rect g_rc_DownLoadDlg_btn_cancel(500, 320, 71, 32);
//Rect g_rc_DownLoadDlg_progress(70, 250, 440, 20);

// 默认字体
LOGFONT g_font_btn_default;		// 默认的按钮字体
LOGFONT g_font_text_default;	// 默认的文字字体
LOGFONT g_font_install_btn_default;		// 默认的install按钮字体

using namespace ns_WAC;

struct UI_UpdateData
{
	bool m_Valid;//是否有效，无效则定时器不会更新界面
	NotifyData m_Info;

	UI_UpdateData()
	{
		m_Valid = false;
	}
};
UI_UpdateData g_UI_UpdateData; //产品信息
UI_UpdateData g_Box_UpdateData; //Box信息
CThreadLock g_locker;
bool SetUI_UpdateData(NotifyData *p)
{
	g_locker.Lock();
	bool bRet = false;
	if(p)
	{
		if(p->nID == g_download_task_id)
		{
			g_UI_UpdateData.m_Valid = true;
			g_UI_UpdateData.m_Info.nID = p->nID;
			g_UI_UpdateData.m_Info.cbTotle = p->cbTotle;
			if(g_UI_UpdateData.m_Info.cbCurPos < p->cbCurPos)
			{
				g_UI_UpdateData.m_Info.cbCurPos = p->cbCurPos;
			}
			g_UI_UpdateData.m_Info.cbSpeedPerSecond = p->cbSpeedPerSecond;
		}
		else if(p->nID == g_wac_task_id)
		{
			g_Box_UpdateData.m_Valid = true;
			g_Box_UpdateData.m_Info.nID = p->nID;
			g_Box_UpdateData.m_Info.cbTotle = p->cbTotle;
			if(g_Box_UpdateData.m_Info.cbCurPos < p->cbCurPos)
			{
				g_Box_UpdateData.m_Info.cbCurPos = p->cbCurPos;
			}
			g_Box_UpdateData.m_Info.cbSpeedPerSecond = p->cbSpeedPerSecond;
		}

		bRet = true;
	}
	else
	{
		g_UI_UpdateData.m_Valid = false;
		g_Box_UpdateData.m_Valid = false;
	}
	g_locker.Unlock ();
	return bRet;
};

void GetUI_UpdateData(UI_UpdateData &Item)
{
	g_locker.Lock ();
	Item.m_Valid = g_UI_UpdateData.m_Valid || g_Box_UpdateData.m_Valid;
	//memcpy(&Item.m_Info, &g_UI_UpdateData.m_Info, sizeof(NotifyData
	//使用两个任务的综合信息更新
	if(g_UI_UpdateData.m_Valid && g_Box_UpdateData.m_Valid)
	{
		Item.m_Info.nID = g_UI_UpdateData.m_Info.nID;
		Item.m_Info.cbTotle = g_UI_UpdateData.m_Info.cbTotle + g_Box_UpdateData.m_Info.cbTotle;
		Item.m_Info.cbCurPos = g_UI_UpdateData.m_Info.cbCurPos + g_Box_UpdateData.m_Info.cbCurPos;
		Item.m_Info.cbSpeedPerSecond = g_UI_UpdateData.m_Info.cbSpeedPerSecond + g_Box_UpdateData.m_Info.cbSpeedPerSecond;
	}
	else if(g_UI_UpdateData.m_Valid && !g_Box_UpdateData.m_Valid)
	{
		Item.m_Info.nID = g_UI_UpdateData.m_Info.nID;
		Item.m_Info.cbTotle = g_UI_UpdateData.m_Info.cbTotle;
		Item.m_Info.cbCurPos = g_UI_UpdateData.m_Info.cbCurPos;
		Item.m_Info.cbSpeedPerSecond = g_UI_UpdateData.m_Info.cbSpeedPerSecond;
	}
	else if(!g_UI_UpdateData.m_Valid && g_Box_UpdateData.m_Valid)
	{
		Item.m_Info.nID = g_Box_UpdateData.m_Info.nID;
		Item.m_Info.cbTotle = g_Box_UpdateData.m_Info.cbTotle;
		Item.m_Info.cbCurPos = g_Box_UpdateData.m_Info.cbCurPos;
		Item.m_Info.cbSpeedPerSecond = g_Box_UpdateData.m_Info.cbSpeedPerSecond;
	}

	g_locker.Unlock ();
}