#include "StdAfx.h"
#include "DownloaderManagerFactory.h"
#include "DownloaderManager.h"

namespace ns_WAC
{
	IDownloaderManager* DownloaderManagerFactory::m_DownloaderManager = NULL;

	DownloaderManagerFactory::DownloaderManagerFactory(void)
	{
	}

	DownloaderManagerFactory::~DownloaderManagerFactory(void)
	{
	}

	IDownloaderManager* DownloaderManagerFactory::get(void)
	{
		if (m_DownloaderManager == NULL)
		{
			m_DownloaderManager = new DownloaderManager;
		}
		return m_DownloaderManager;
	}

	void DownloaderManagerFactory::release()
	{
		if(m_DownloaderManager != NULL)
		{
			delete m_DownloaderManager;
			m_DownloaderManager = NULL;
		}
	}
}