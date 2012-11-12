#pragma once
#include "IDownloaderManager.h"
namespace ns_WAC
{
	class DownloaderManagerFactory
	{
	public:
		DownloaderManagerFactory(void);
		~DownloaderManagerFactory(void);
		static IDownloaderManager* get(void);
		static void release();

	private:
		static IDownloaderManager* m_DownloaderManager;
	};
}
