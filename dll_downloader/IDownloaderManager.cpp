#include "StdAfx.h"
#include "IDownloaderManager.h"
#include "DownloaderManagerFactory.h"

namespace ns_WAC
{
	IDownloaderManager::IDownloaderManager(void)
	{
	}


	IDownloaderManager::~IDownloaderManager(void)
	{
	}
}

//ֻ��Ҫ���ã�����Ҫ�ͷţ�������delete
DOWNLOADERMANAGER_API ns_WAC::IDownloaderManager* CreateDownloadMGR(void)
{
	return ns_WAC::DownloaderManagerFactory::get();
}
