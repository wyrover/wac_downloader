#pragma once
#include "idownloadermanager.h"
#include "PublicFunction.h"

namespace ns_WAC
{

	typedef struct _DownloadNotifyPara
	{
		int nIndex;
		int nSerialNum;
		UINT nNotityType;
		LPVOID lpNotifyData;
	} t_DownloadNotifyPara;

	struct NotifyData
	{
		int     nID;				//����id
		__int64 cbTotle;			//�����ļ����ܴ�С
		__int64 cbCurPos;           //�Ѿ������ļ��Ĵ�С
		int     cbSpeedPerSecond;	//���ص��ٶ�

		NotifyData()
		{
			nID = cbSpeedPerSecond = 0;
			cbTotle = cbCurPos = 0;
		}
	};

	class CTaskManager;

	enum
	{
		NOTIFY_TYPE_HAS_GOT_REMOTE_FILENAME,				// �Ѿ�ȡ��Զ��վ���ļ���, �������ص��ļ����ض���ʱ�ŷ��͸�֪ͨ��lpNotifyData Ϊ LPCTSTR ���͵��ļ����ַ���ָ��
		NOTIFY_TYPE_HAS_GOT_REMOTE_FILESIZE,				// �Ѿ�ȡ��Զ��վ���ļ���С��lpNotifyData Ϊ int ���͵��ļ���С
		NOTIFY_TYPE_HAS_END_DOWNLOAD,						// �Ѿ��������أ�lpNotifyData Ϊ ENUM_DOWNLOAD_RESULT ���͵����ؽ��
		NOTIFY_TYPE_HAS_GOT_NEW_DOWNLOADED_DATA,            // �Ѿ����ص�

		NOTIFY_TYPE_HAS_GOT_THREAD_WILL_DOWNLOAD_SIZE,		// �Ѿ���ȡ��ĳ�̱߳�����Ҫ���صĴ�С��lpNotifyData Ϊ int ���͵���Ҫ���صĴ�С
		NOTIFY_TYPE_HAS_GOT_THREAD_DOWNLOADED_SIZE,			// �Ѿ���ȡ��ĳ�߳������ص����ݴ�С��lpNotifyData Ϊ int ���͵������صĴ�С
		NOTIFY_TYPE_HAS_GOT_THREAD_CACHE_SIZE,			    // �Ѿ���ȡ��ĳ�߳������صĻ������ݴ�С��lpNotifyData Ϊ int ���͵������صĴ�С
		NOTIFY_TYPE_HAS_START,	//��ʼ����
	};

	typedef enum _DownloadResult
	{
		ENUM_DOWNLOAD_RESULT_SUCCESS,
		ENUM_DOWNLOAD_RESULT_FAILED,
	} ENUM_DOWNLOAD_RESULT;

	class IObserver
	{
	public:
		virtual void OnNotify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam) = 0;
	};

	class DownloaderManager : public IDownloaderManager, public IObserver
	{
	public:
		DownloaderManager(void);
		~DownloaderManager(void);

		//static IDownloaderManager* get_instance();

		BOOL Pause(int nTaskID);
		BOOL Continue(int nTaskID);
		int	AddTask(const wchar_t *pszUrl,
					const wchar_t *pszSavePath,
					const wchar_t *pszSaveFileName,
					int	  PID);
		BOOL RemoveTask(int nTaskID);
		BOOL QueryTask(int nTaskID, DownloadTask* sDownloadTask);
		BOOL Cancel(int nTaskID);
		virtual void		SetCallBack(IDownloaderManagerCallBack*);
		virtual void		SetCallBack(HWND hWnd, UINT nMsgID);
		virtual void		SetCallBack(ICallBack* call_back);
		virtual void		ReleaseCallBack();

		virtual unsigned	GetTaskCount();
		//virtual BOOL		GetAllTask(__out DownloadTask *pDownload, int nBufferSize);
		virtual BOOL		Clear();	

	private:
		//static IDownloaderManager* s_sinalton;

	private:
		void OnNotify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam);

		CTaskManager *m_pDownloadCntl;								// ���ع�����ָ��
		IDownloaderManagerCallBack *m_pcb;
		ICallBack* m_icb;
		HWND m_hCallbackWnd;										// ���ջص���Ϣ�Ĵ��ھ��
		UINT m_nCallBackMsgID;										// �������ڽ��ջص���Ϣ����ϢID
	};
}
