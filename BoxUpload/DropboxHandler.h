#pragma once
#include <string>
#include "WinHttpsUtil.h"
#include <iostream>
#include <strstream>  
#include <sstream>  
#include <string>  

using namespace std;

class DropboxHandler
{
public:
	wstring accessToken;
	inline void PutFile(wstring filepath, BYTE* data, unsigned int dataSize);

};

std::wstring IntToWstring(unsigned int i)
{
	std::wstringstream ss;
	ss << i;
	return ss.str();
}

void DropboxHandler::PutFile(wstring filepath, BYTE* data, unsigned int dataSize)
{
	// Set URL.
	WebClient client(L"https://content.dropboxapi.com/2/files/upload");
	client.SetProxy(L"127.0.0.1:8080");

	wstring command = L"{\"path\": \"/" + filepath + L"\", \"mode\": \"overwrite\", \"autorename\": false, \"mute\": true}";
	wstring newHeadhers = L"User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:49.0) Gecko/20100101 Firefox/49.0\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Content-Length: " + IntToWstring(dataSize) + L"\r\n"
		"Dropbox-API-Arg: " + command + L"\r\n"
		"Authorization: Bearer " + accessToken;
	client.SetAdditionalRequestHeaders(newHeadhers);

	client.SetAdditionalDataToSend(data, dataSize);
	// Send HTTP request, a GET request by default.
	client.SendHttpRequest();

	wstring StatusCode = client.GetResponseStatusCode();

	// The response content.
	wstring httpResponseContent = client.GetResponseContent();

	wstring sadf = L"";
}
