// BoxUpload.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "DropboxHandler.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <wtypes.h>
using namespace std;

int main(int argc, char* argv[])
{
	DropboxHandler dropbox;
	dropbox.accessToken = L"";


	wstring filename = L"";

	ifstream t;
	int length;

	t.open(filename);
	t.seekg(0, ios::end);
	length = t.tellg();
	t.seekg(0, ios::beg);
	char* buffer = new char[length];
	t.read(buffer, length);
	t.close();

	BYTE* lpBuffer = (BYTE*)buffer;

	dropbox.PutFile(filename, lpBuffer, length);

	return 0;
}