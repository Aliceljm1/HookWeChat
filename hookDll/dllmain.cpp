// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <atlstr.h> //CString的头文件

HMODULE GetWechatWin()
{
	return GetModuleHandle(L"WeChatWin.dll");
}

struct wxMsg
{
	wchar_t* pStr;
	int strLen;
	int strLen2;
	char buff[0x34];
};
struct wxId
{
	wchar_t* pStr;
	int strLen;
	int strLen2;
	char buff[0x8];
};
struct wxAt {
	wxId* atid;
	DWORD p1;
	DWORD p2;
	int end = 0;
};

void test()
{
	HMODULE hModule = GetWechatWin();

	// 偏移量
	DWORD WXIDOFFSET = 0x25357A0;
	DWORD PHONEOFFSET = 0x2535B70;

	DWORD pwxid = (DWORD)hModule + WXIDOFFSET;
	DWORD pphone = (DWORD)hModule + PHONEOFFSET;

	// 窄转宽
	CA2W id((char*)pwxid);
	wchar_t* wxid = (wchar_t*)id;
	CA2W ph((char*)pphone);
	wchar_t* phone = (wchar_t*)ph;

	CString s;
	s.Format(_T("hook addr=%x,\nwxid=%s,\nphone num=%s"), hModule, wxid, phone);

	MessageBox(NULL, s.GetBuffer(), L"Hook WeChatWin.dll", MB_OK);
}

void SendWechatMessage(wchar_t* wxid, wchar_t* msg)
{
	__asm {
		push 0x1;
		mov eax, 0;
		push eax;
	}
	
	wxMsg text = { 0 };
	text.pStr = msg;
	text.strLen = wcslen(msg);
	text.strLen2 = wcslen(msg) * 2;
	char* pWxmsg = (char*)&text.pStr;

	__asm {
		push 1;
		mov eax, 0;
		push eax;
		mov edi, pWxmsg;
		push edi;
	}

	wxMsg id = { 0 };
	id.pStr = wxid;
	id.strLen = wcslen(wxid);
	id.strLen2 = wcslen(wxid) * 2;
	char* pWxid = (char*)&id.pStr;

	char buff[0x81C] = { 0 };

	__asm {
		push 1;
		mov eax, 0;
		push eax;
		mov edi, pWxmsg;
		push edi;
		mov edx, pWxid;
		lea ecx, buff;
	}

	DWORD dwSendCallAddr = (DWORD)GetWechatWin() + (0x6658d2e0 - 0x65fc0000);
	__asm {
		push 1;
		mov eax, 0;
		push eax;
		mov edi, pWxmsg;
		push edi;
		mov edx, pWxid;
		lea ecx, buff;
		call dwSendCallAddr;
		add esp, 0xC;
	}
}

void SendWechatMessage(wchar_t* wxid, wchar_t* msg, wchar_t* atsb)
{
	//拿到发送消息的call的地址
	DWORD dwSendCallAddr = (DWORD)GetWechatWin() + (0x6658d2e0 - 0x65fc0000);

	//微信ID/群ID
	wxMsg id = { 0 };
	id.pStr = wxid;
	id.strLen = wcslen(wxid);
	id.strLen2 = wcslen(wxid) * 2;

	//消息内容
	wxMsg text = { 0 };
	text.pStr = msg;
	text.strLen = wcslen(msg);
	text.strLen2 = wcslen(msg) * 2;

	wxAt at = { 0 };
	if (atsb != NULL) {

		wxId atid = { 0 };
		atid.pStr = atsb;
		atid.strLen = wcslen(atsb);
		atid.strLen2 = wcslen(atsb) * 2;
		at.atid = (wxId*)&atid.pStr;
		at.p1 = (DWORD)(at.atid + sizeof(atsb) / 4);
		at.p2 = at.p1;
	}
	//取出微信ID和消息的地址
	char* pWxid = (char*)&id.pStr;
	char* pWxmsg = (char*)&text.pStr;

	char buff[0x81C] = { 0 };
	//调用微信发送消息call
	__asm {
		push 1;
		lea eax, at;
		push eax;
		mov edi, pWxmsg;
		push edi;
		mov edx, pWxid;
		lea ecx, buff;
		call dwSendCallAddr;
		add esp, 0xC;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		/*我们的代码，注入DLL后会执行*/
		// MessageBox(0, L"成功潜入微信内部！", L"报告主人", NULL);
		test();
		SendWechatMessage((wchar_t*)L"wxid_q2mirynz7h6622", (wchar_t*)L"DLL成功注入，并向你发送了一条消息。");
		// SendWechatMessage((wchar_t*)L"22526049857@chatroom", (wchar_t*)L"@name DLL成功注入，并向你发送了一条消息。", (wchar_t*)L"wxid_ivxugfki4phi22");
	case DLL_THREAD_ATTACH:

	case DLL_THREAD_DETACH:

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
