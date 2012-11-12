// dllmain.cpp : DllMain 的实现。

#include "stdafx.h"
#include "resource.h"
#include "dllmain.h"

// DLL 入口点
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	return true; 
}
