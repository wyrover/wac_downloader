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

//只需要调用，不需要释放，即不能delete
DOWNLOADERMANAGER_API ns_WAC::IDownloaderManager* CreateDownloadMGR(void)
{
	return ns_WAC::DownloaderManagerFactory::get();
}
