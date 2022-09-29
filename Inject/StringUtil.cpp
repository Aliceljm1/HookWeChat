#include <windows.h>
#include <tchar.h>
#include <sstream>
#include <atlstr.h>
#include <iostream>

// Õ­±ä¿í
wchar_t* a2w(char* p_a) {

	int need_w_char = MultiByteToWideChar(CP_ACP, 0, p_a, -1, NULL, 0);
	if (need_w_char < 0) return NULL;

	wchar_t* p_w = new wchar_t[need_w_char];
	wmemset(p_w, 0, need_w_char);
	MultiByteToWideChar(CP_ACP, 0, p_a, -1, p_w, need_w_char);
	return  p_w;
}


// ¿í±äÕ­
char* w2a(wchar_t* p_w) {

	int need_a_char = WideCharToMultiByte(CP_ACP, 0, p_w, -1, NULL, 0, NULL, NULL);
	if (need_a_char < 0) return NULL;

	char* p_a = new char[need_a_char];
	memset(p_a, 0, need_a_char);
	WideCharToMultiByte(CP_ACP, 0, p_w, -1, p_a, need_a_char, NULL, NULL);
	return  p_a;
}

void msg(const wchar_t* str) {
	MessageBox(NULL, str, _T("±êÌâ"), MB_OK);
}

void test(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {


	char s_a[] = "¹þ¹þ¹þ¹þÕ­";
	wchar_t s_w[] = L"¹þ¹þ¹þ¹þ¿í";

	wchar_t* p_a = a2w(s_a);
	wchar_t* p_w = a2w(w2a(s_w));

	msg(p_w);
	msg(p_a);

	delete[] p_a;
	delete[] p_w;

	// Õ­±ä¿í
	CA2W a2wObj(s_a);
	wchar_t* p_s_a = (wchar_t*)a2wObj;

	// ¿í±äÕ­
	CW2A w2aObj(s_w);
	char *p_s_w = (char*)w2aObj;

	msg(p_s_a);

	CString str;
	str += "CString×Ö·û´®";

	str.Format(_T("hello %s"), str.GetBuffer());
	msg(str.GetBuffer());
}