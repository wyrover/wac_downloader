#pragma once

//保存任务的下载跟踪信息
enum TaskStatus
{
	UnknownStatus = 0,
	DownFailed = 1,
	DownPaused = 2,
	DownSucced = 3,
	InstallSucced = 4,
	InstallFailed = 5,
};

struct TaskTrack
{
	std::wstring download_start;
	std::wstring download_end;
	std::wstring download_status;

	std::wstring install_start;
	std::wstring install_end;
	std::wstring install_status;
};