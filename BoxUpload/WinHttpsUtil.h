#pragma once
#include <comutil.h>
#include <windows.h>
#include <Winhttp.h>
#include <string>
#include <vector>
using namespace std;
#pragma comment(lib, "Winhttp.lib")

typedef bool (*PROGRESSPROC)(double);
// 重试次数
static const unsigned int Int_RetryTimes = 3;
// 初始 10 KB 临时缓冲区，如果不够，则加倍
static const int Int_BufferSize = 10240;

class WebClient 
{
public:
	inline WebClient(const wstring& url, PROGRESSPROC progressProc = NULL);
	inline ~WebClient(void);

	// 设置代理
	inline bool SetProxy(const wstring& proxy);
	// 发送的数据
	inline bool SetAdditionalDataToSend(BYTE* data, unsigned int dataSize);

    inline bool SetTimeouts(
        unsigned int resolveTimeout = 0,
        unsigned int connectTimeout = 60000,
        unsigned int sendTimeout = 30000,
        unsigned int receiveTimeout = 30000
    );

    inline bool SetAdditionalRequestHeaders(const wstring& additionalRequestHeaders);

	// 发起请求，默认为 GET
	inline bool SendHttpRequest();

	// 获取响应状态码
	inline wstring GetResponseStatusCode(void);
	// 获取响应内容
	inline wstring GetResponseContent(void);

	

private:
	inline WebClient(const WebClient& other);

    HINTERNET m_sessionHandle;
    wstring m_requestURL;
    wstring m_requestHost;
    wstring m_responseHeader;
    wstring m_responseContent;
    BYTE* m_pDataToSend;
    unsigned int m_dataToSendSize;
    BYTE* m_pResponse;
    wstring m_proxy;
    wstring m_additionalRequestHeaders;
    wstring m_statusCode;

    unsigned int m_resolveTimeout;
    unsigned int m_connectTimeout;
    unsigned int m_sendTimeout;
    unsigned int m_receiveTimeout;

};

WebClient::WebClient(const wstring& url, PROGRESSPROC progressProc)

    : m_requestURL(url),
    m_sessionHandle(NULL),
    m_responseHeader(L""),
    m_responseContent(L""),
    m_requestHost(L""),
    m_pResponse(NULL),
    m_pDataToSend(NULL),
    m_dataToSendSize(0),
    m_proxy(L""),
    m_statusCode(L""),


    m_resolveTimeout(0),
    m_connectTimeout(60000),
    m_sendTimeout(30000),
    m_receiveTimeout(30000)
{
}

WebClient::~WebClient(void)
{
    if (m_pResponse != NULL)
    {
        delete[] m_pResponse;
    }
    if (m_pDataToSend != NULL)
    {
        delete[] m_pDataToSend;
    }

    if (m_sessionHandle != NULL)
    {
        ::WinHttpCloseHandle(m_sessionHandle);
    }
}

/// <summary>
/// 获取响应内容
/// </summary>
wstring WebClient::GetResponseContent(void)
{
    return m_responseContent;
}

/// <summary>
/// 获取相应状态码
/// </summary>
wstring WebClient::GetResponseStatusCode(void)
{
    return m_statusCode;
}

/// <summary>
/// 发起请求
/// </summary>
bool WebClient::SendHttpRequest()
{
    if (m_requestURL.size() <= 0)
    {
        return false;
    }

    bool bRetVal = true;

    if (m_sessionHandle == NULL)
    {
        m_sessionHandle = ::WinHttpOpen(L"rowteam",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
        if (m_sessionHandle == NULL)
        {
            return false;
        }
    }

    ::WinHttpSetTimeouts(m_sessionHandle,
        m_resolveTimeout,
        m_connectTimeout,
        m_sendTimeout,
        m_receiveTimeout);

    wchar_t szHostName[MAX_PATH] = L"";
    wchar_t szURLPath[MAX_PATH * 5] = L"";
    URL_COMPONENTS urlComp;
    memset(&urlComp, 0, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = szHostName;
    urlComp.dwHostNameLength = MAX_PATH;
    urlComp.lpszUrlPath = szURLPath;
    urlComp.dwUrlPathLength = MAX_PATH * 5;
    urlComp.dwSchemeLength = 1; // None zero

    if (::WinHttpCrackUrl(m_requestURL.c_str(), m_requestURL.size(), 0, &urlComp))
    {
        m_requestHost = szHostName;
        HINTERNET hConnect = NULL;
        hConnect = ::WinHttpConnect(m_sessionHandle, szHostName, urlComp.nPort, 0);
        if (hConnect != NULL)
        {
            DWORD dwOpenRequestFlag = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
            HINTERNET hRequest = NULL;
            hRequest = ::WinHttpOpenRequest(hConnect,
                L"POST",
                urlComp.lpszUrlPath,
                NULL,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                dwOpenRequestFlag);
            if (hRequest != NULL)
            {
                // 如果是 HTTPS，那么客户端很容易受到无效证书的影响
                if (urlComp.nScheme == INTERNET_SCHEME_HTTPS)
                {
                    DWORD options = SECURITY_FLAG_IGNORE_CERT_CN_INVALID
                        | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
                        | SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                    ::WinHttpSetOption(hRequest,
                        WINHTTP_OPTION_SECURITY_FLAGS,
                        (LPVOID)&options,
                        sizeof(DWORD));
                }

                bool bGetReponseSucceed = false;
                unsigned int iRetryTimes = 0;

                // 如果失败重试 3 次.
                while (!bGetReponseSucceed && iRetryTimes++ < Int_RetryTimes)
                {
                    // 设置 Headers
                    if (m_additionalRequestHeaders.size() > 0)
                    {
                        ::WinHttpAddRequestHeaders(hRequest, m_additionalRequestHeaders.c_str(), m_additionalRequestHeaders.size(), WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON);
                    }
   
                    if (m_proxy.size() > 0)
                    {
                        WINHTTP_PROXY_INFO proxyInfo;
                        memset(&proxyInfo, 0, sizeof(proxyInfo));
                        proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                        wchar_t szProxy[MAX_PATH] = L"";
                        wcscpy_s(szProxy, MAX_PATH, m_proxy.c_str());
                        proxyInfo.lpszProxy = szProxy;

                        WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo));
                    }

                    bool bSendRequestSucceed = false;
                    if (::WinHttpSendRequest(hRequest,
                        WINHTTP_NO_ADDITIONAL_HEADERS,
                        0,
                        WINHTTP_NO_REQUEST_DATA,
                        0,
                        0,
                        NULL))
                    {
                        bSendRequestSucceed = true;
                    }
                    else
                    {
                        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;
                        memset(&proxyConfig, 0, sizeof(proxyConfig));
                        // 从 IE 设置中查询代理信息，如果有，请设置代理
                        if (::WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig))
                        {
                            if (proxyConfig.lpszAutoConfigUrl != NULL)
                            {
                                WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions;
                                memset(&autoProxyOptions, 0, sizeof(autoProxyOptions));
                                autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT | WINHTTP_AUTOPROXY_CONFIG_URL;
                                autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP;
                                autoProxyOptions.lpszAutoConfigUrl = proxyConfig.lpszAutoConfigUrl;
                                autoProxyOptions.fAutoLogonIfChallenged = TRUE;
                                autoProxyOptions.dwReserved = 0;
                                autoProxyOptions.lpvReserved = NULL;

                                WINHTTP_PROXY_INFO proxyInfo;
                                memset(&proxyInfo, 0, sizeof(proxyInfo));

                                if (::WinHttpGetProxyForUrl(m_sessionHandle, m_requestURL.c_str(), &autoProxyOptions, &proxyInfo))
                                {
                                    if (::WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo)))
                                    {
                                        if (::WinHttpSendRequest(hRequest,
                                            WINHTTP_NO_ADDITIONAL_HEADERS,
                                            0,
                                            WINHTTP_NO_REQUEST_DATA,
                                            0,
                                            0,
                                            NULL))
                                        {
                                            bSendRequestSucceed = true;
                                        }
                                    }
                                    if (proxyInfo.lpszProxy != NULL)
                                    {
                                        ::GlobalFree(proxyInfo.lpszProxy);
                                    }
                                    if (proxyInfo.lpszProxyBypass != NULL)
                                    {
                                        ::GlobalFree(proxyInfo.lpszProxyBypass);
                                    }
                                }
                            }
                            else if (proxyConfig.lpszProxy != NULL)
                            {
                                WINHTTP_PROXY_INFO proxyInfo;
                                memset(&proxyInfo, 0, sizeof(proxyInfo));
                                proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                                wchar_t szProxy[MAX_PATH] = L"";
                                wcscpy_s(szProxy, MAX_PATH, proxyConfig.lpszProxy);
                                proxyInfo.lpszProxy = szProxy;

                                if (proxyConfig.lpszProxyBypass != NULL)
                                {
                                    wchar_t szProxyBypass[MAX_PATH] = L"";
                                    wcscpy_s(szProxyBypass, MAX_PATH, proxyConfig.lpszProxyBypass);
                                    proxyInfo.lpszProxyBypass = szProxyBypass;
                                }

                                ::WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo));
                            }

                            if (proxyConfig.lpszAutoConfigUrl != NULL)
                            {
                                ::GlobalFree(proxyConfig.lpszAutoConfigUrl);
                            }
                            if (proxyConfig.lpszProxy != NULL)
                            {
                                ::GlobalFree(proxyConfig.lpszProxy);
                            }
                            if (proxyConfig.lpszProxyBypass != NULL)
                            {
                                ::GlobalFree(proxyConfig.lpszProxyBypass);
                            }
                        }
                    }

                    // 如果请求发送成功
                    if (bSendRequestSucceed)
                    {
                        // 发送数据
                        if (m_pDataToSend != NULL)
                        {
                            DWORD dwWritten = 0;
                            ::WinHttpWriteData(hRequest,
                                m_pDataToSend,
                                m_dataToSendSize,
                                &dwWritten);
                        }

                        // 解析数据
                        if (::WinHttpReceiveResponse(hRequest, NULL))
                        {
                            DWORD dwSize = 0;
                            BOOL bResult = FALSE;
                            // 获取状态码
                            bResult = ::WinHttpQueryHeaders(hRequest,
                                WINHTTP_QUERY_STATUS_CODE,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                NULL,
                                &dwSize,
                                WINHTTP_NO_HEADER_INDEX);
                            if (bResult || (!bResult && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)))
                            {
                                wchar_t* szStatusCode = new wchar_t[dwSize];
                                if (szStatusCode != NULL)
                                {
                                    memset(szStatusCode, 0, dwSize * sizeof(wchar_t));
                                    if (::WinHttpQueryHeaders(hRequest,
                                        WINHTTP_QUERY_STATUS_CODE,
                                        WINHTTP_HEADER_NAME_BY_INDEX,
                                        szStatusCode,
                                        &dwSize,
                                        WINHTTP_NO_HEADER_INDEX))
                                    {
                                        m_statusCode = szStatusCode;
                                    }
                                    delete[] szStatusCode;
                                }
                            }

                            unsigned int iMaxBufferSize = Int_BufferSize;
                            unsigned int iCurrentBufferSize = 0;
                            if (m_pResponse != NULL)
                            {
                                delete[] m_pResponse;
                                m_pResponse = NULL;
                            }
                            m_pResponse = new BYTE[iMaxBufferSize];
                            if (m_pResponse == NULL)
                            {
                                bRetVal = false;
                                break;
                            }
                            memset(m_pResponse, 0, iMaxBufferSize);
                            
                            bGetReponseSucceed = true;
                        }
                    }
                } // while
                if (!bGetReponseSucceed)
                {
                    bRetVal = false;
                }

                ::WinHttpCloseHandle(hRequest);
            }
            ::WinHttpCloseHandle(hConnect);
        }
    }

    return bRetVal;
}

/// <summary>
/// 设置要发送的附加数据
/// </summary>
bool WebClient::SetAdditionalDataToSend(BYTE* data, unsigned int dataSize)
{
    if (data == NULL || dataSize < 0)
    {
        return false;
    }

    if (m_pDataToSend != NULL)
    {
        delete[] m_pDataToSend;
    }
    m_pDataToSend = NULL;
    m_pDataToSend = new BYTE[dataSize];
    if (m_pDataToSend != NULL)
    {
        memcpy(m_pDataToSend, data, dataSize);
        m_dataToSendSize = dataSize;
        return true;
    }

    return false;
}

/// <summary>
/// 设置 Headers
/// </summary>
bool WebClient::SetAdditionalRequestHeaders(const wstring& additionalRequestHeaders)
{
    m_additionalRequestHeaders = additionalRequestHeaders;

    return true;
}

/// <summary>
/// 设置代理（http/https）
/// </summary>
bool WebClient::SetProxy(const wstring& proxy)
{
    m_proxy = proxy;

    return true;
}

/// <summary>
/// 设置超时
/// </summary>
bool WebClient::SetTimeouts(unsigned int resolveTimeout,
    unsigned int connectTimeout,
    unsigned int sendTimeout,
    unsigned int receiveTimeout)
{
    m_resolveTimeout = resolveTimeout;
    m_connectTimeout = connectTimeout;
    m_sendTimeout = sendTimeout;
    m_receiveTimeout = receiveTimeout;

    return true;
}