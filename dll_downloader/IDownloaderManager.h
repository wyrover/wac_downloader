#pragma once

#include <Windows.h>
#include "wac_downloader_i.h"
namespace ns_WAC
{
	//////////////////////////////////////////////////////////////////////////
	/************************************************************************/
	// ����������
	//	1.�����񣬶��̣߳�֧�ֶϵ�����
	//	2.֧��Э������ : FTP , HTTP
	/************************************************************************/
	//////////////////////////////////////////////////////////////////////////
	// ��������
	const int MAX_URL_LENGTH = 512;
	struct DownloadTask
	{
		int			nID;				//����id

		__int64     cbTotle,			//�����ļ����ܴ�С
			cbCurPos;           //�Ѿ������ļ��Ĵ�С
		int			cbSpeedPerSecond;	//���ص��ٶ�
		wchar_t		szUrl[MAX_URL_LENGTH];         //���������url
		wchar_t		szSavePath[MAX_URL_LENGTH];    //��������ı���·��
		wchar_t		szSaveFileName[MAX_URL_LENGTH];//�������񱣴���ļ���
	};

	typedef enum NsResultEnum
	{
		RS_SUCCESS = 0,						// Operation success
		RS_NO_NEED_DOWNLOAD,                // ��������

		RS_ERR_CONNECT_FAILED,				// ����ʧ��
		RS_ERR_RCV_FAILED,					// ����ʧ��
		RS_ERR_SEND_FAILED,					// ����ʧ��
		RS_ERR_GET_WEBINFO_FAILED,          // ��ȡվ����Ϣʧ��

		RS_ERR_TASK_ALLOC_FAILED,           // �������ʧ��
		RS_ERR_BUFSIZE,						// Buffer size out of band
		RS_INVALID_PARAMETER,				// �����Ƿ�
		RS_ERR_FILE_OPRATION_FAILD,			// �ļ�����ʧ��
		RS_ERR_DISK_SPACE_NOT_ENOUGH,		// ���̿ռ䲻��
		RS_ERR_MD5_CHECK_FAILED,			// MD5У��ʧ��

		RS_ERR_FILE_SIZE_INCORRECT,			// �ļ���С����ȷ�����������߳��˳����Բ���������ɣ�
		RS_ERR_USER_CANCEL,					// �û���ȡ��
		RS_ERR_DOWNLOAD_FAILED,             // ����ʧ��
		RS_ERR_UNKOWN,
	};

	struct IDownloaderManagerCallBack;
	class IDownloaderManager
	{
	public:
		IDownloaderManager(void);
		virtual ~IDownloaderManager(void) = 0;

		//=============================================================================
		//
		// �������ܣ�	��ͣ��������
		// ������		nTaskID����Ҫ��ͣ������id
		// ����ֵ��		BOOL
		// ��ע��		2012-9-6
		//
		//=============================================================================
		virtual BOOL		Pause(int nTaskID) = 0;

		//=============================================================================
		//
		// �������ܣ�	������������
		// ������		nTaskID����Ҫ�������ص�����id
		// ����ֵ��		BOOL
		// ��ע��		2012-9-6
		//
		//=============================================================================
		virtual BOOL		Continue(int nTaskID) = 0;

		//=============================================================================
		//
		// �������ܣ�	�����������
		// ������		pszUrl��         ���������url
		//      		pszSavePath��    ��������ı���·��
		//      		pszSaveFileName��������ɺ���ļ���
		//      		iPID��     ��������PID
		// ����ֵ��		int:             ���������id��������ͣ�ͼ����� С��0�������ʧ��
		// ��ע��		2012-9-6
		//
		//=============================================================================
		virtual int			AddTask(const wchar_t *pszUrl,
									const wchar_t *pszSavePath,
									const wchar_t *pszSaveFileName,
									int	  iPID) = 0;

		//=============================================================================
		//
		// �������ܣ�	ȡ��һ���������񣬲�ɾ����Ӧ�ļ�
		// ������		nTaskID����Ҫɾ������������id
		// ����ֵ��		BOOL
		// ��ע��		2012-9-6
		//
		//=============================================================================
		virtual BOOL		RemoveTask(int nTaskID) = 0;

		//=============================================================================
		//
		// �������ܣ�	��ѯ����������������
		// ������		nTaskID��		��������id
		//				DownloadTask*�� ������������Խṹ�� ��������new��
		// ����ֵ��		BOOL
		// ��ע��		2012-9-6
		//
		//=============================================================================
		virtual BOOL		QueryTask(int nTaskID, DownloadTask* sDownloadTask) = 0; 

		//=============================================================================
		//
		// �������ܣ�	ȡ����������
		// ������		nTaskID��		��������id
		// ����ֵ��		BOOL
		// ��ע��		2012-9-6
		//
		//=============================================================================
		virtual BOOL		Cancel(int nTaskID) = 0; 

		//=============================================================================
		//
		// �������ܣ�	���ûص�����
		// ������		IDownloaderManagerCallBack����ָ��
		// ����ֵ��		void
		// ��ע��		2012-9-6
		//
		//=============================================================================
		virtual void		SetCallBack(IDownloaderManagerCallBack* pcb) = 0; 

		virtual void		SetCallBack(HWND hWnd, UINT nMsgID) = 0;
		virtual void		SetCallBack(ICallBack* call_back) = 0;
		virtual void		ReleaseCallBack() = 0;
	};

		//////////////////////////////////////////////////////////////////////////
	// �ص�
	struct IDownloaderManagerCallBack
	{
		/*
		// �û����ýӿں� �����ִ�н��
		virtual BOOL OnDelTask(int nTaskID) {};								// ɾ�������ִ�н��
		virtual BOOL OnPause(int nTaskID) {};								// ��ͣ�����ִ�н��
		virtual BOOL OnCotinue(int nTaskID) {};								// ���������ִ�н��
		*/


		virtual void OnGetFileName(int nTaskID, WCHAR *szFileName) {};		// ��ȡ���ļ���
		virtual void OnGetFileSize(int nTaskID, __int64 nTotalSize) {};		// ��ȡ���ļ���С

		// ����ִ��״�����û���Ҫ�õ�֪ͨ��
		virtual void OnUpdate(int nTaskID,
							  __int64 nCurPos,
							  __int64 nTotalSize,
							  int nSpeed) {};								// �������ؽ��ȱ仯

		virtual void OnComplete(int nTaskID) {};							// �����������
		virtual void OnFail(int nTaskID, NsResultEnum nsError) {};			// ��������ʧ��

		virtual void OnTest(int nTaskID, int nThreadID, int nType, __int64 nData) {};
		virtual void onStart(int nTaskID) {};
	};
}

#ifdef DOWNLOADERMANAGER_EXPORTS
#define DOWNLOADERMANAGER_API extern "C" __declspec(dllexport)
#else
#define DOWNLOADERMANAGER_API extern "C" __declspec(dllimport)
#endif

DOWNLOADERMANAGER_API ns_WAC::IDownloaderManager* CreateDownloadMGR(void);
