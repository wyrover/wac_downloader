#pragma once

#include <string>
//Server 进程管道名称
#define PIPE_NAME_SVR   L"\\\\.\\Pipe\\Wondershare\\UC\\SVR"
//Download 进程管道名称
#define PIPE_NAME_DWN  L"\\\\.\\Pipe\\Wondershare\\UC\\DWN"

/*
* 使用命名管道实现IPC
*/
#pragma pack(4)
struct T_Message{
};

struct T_Message_Install : public T_Message
{
	int pid;
	int install_return_;
};

struct T_Message_Tips : public T_Message
{
	int width;
	int height;
	int duration; //自动显示时长
	char url[256];
};


template <typename T>
class CIPCSender
{
public:
	CIPCSender();
	~CIPCSender(void);
	bool init(TCHAR *pipeName);
	bool SendData(T msg);
private:
	HANDLE m_hPipe;

};


template <typename T>
CIPCSender<T>::CIPCSender()
{
	m_hPipe = NULL;
}

template <typename T>
CIPCSender<T>::~CIPCSender(void)
{
	if(m_hPipe!=NULL){
		CloseHandle(m_hPipe); // 关闭管道句柄
	}
}

template <typename T>
bool CIPCSender<T>::init(TCHAR *pipeName)
{
	// 等待与服务器的连接
	if (WaitNamedPipe(pipeName, NMPWAIT_WAIT_FOREVER) == FALSE)
	{
		//"等待连接失败!"; 
		return false;
	}
	// 打开已创建的管道句柄
	m_hPipe = CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE, 
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	return (m_hPipe!=NULL);

}


template <typename T>
bool CIPCSender<T>::SendData(T msg)
{
	//数据生成
	std::wstring  Message = L"[test data send from client]"; // 要发送的数据
	DWORD WriteNum; // 发送的是数据长度


	if (m_hPipe == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG
		MessageBox(NULL,L"open pipe failied",L"tip",0);
#endif
		return false;
	}

	// 向管道写入数据
	if (WriteFile(m_hPipe,&msg,sizeof (msg), &WriteNum, NULL) == FALSE)
	{
#ifdef _DEBUG
		//MessageBox(NULL,L"write data failed!",L"tip",0);
#endif

		return false;
	} else {
#ifdef _DEBUG
		//MessageBox(NULL,Message.c_str(),L"send success",0);
#endif
		return true;
	}


}
