#include <string>
#include <vector>
#include <Windows.h>
#include <Winhttp.h>
extern "C" {
#include "windows/miniz.c"
#include "windows/mini_gzip.c"
}
#pragma comment (lib, "Winhttp.lib")
#include <codecvt>

bool ungzip(char*source, size_t len, std::vector<char> &out)
{
  struct mini_gzip gz;
  int result = mini_gz_start(&gz, source, len);
  if (result != 0) {
    return false;
  }

  void *memout = NULL;
  size_t outLength;
  result = mini_gz_unpack(&gz, &memout, &outLength);
  if (result != 0) {
    return false;
  }
  out.resize(outLength);
  memcpy(out.data(), memout, outLength);
  return true;
}

bool urlGet(const std::string &url, std::vector<char> &stream) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

  URL_COMPONENTS uc;
  wchar_t Scheme[256];
  wchar_t HostName[256];
  wchar_t UserName[256];
  wchar_t Password[256];
  wchar_t UrlPath[1024];
  wchar_t ExtraInfo[256];
  uc.dwStructSize = sizeof(uc);
  uc.lpszScheme = Scheme;
  uc.lpszHostName = HostName;
  uc.lpszUserName = UserName;
  uc.lpszPassword = Password;
  uc.lpszUrlPath = UrlPath;
  uc.lpszExtraInfo = ExtraInfo;

  uc.dwSchemeLength = _countof(Scheme);
  uc.dwHostNameLength = _countof(HostName);
  uc.dwUserNameLength = _countof(UserName);
  uc.dwPasswordLength = _countof(Password);
  uc.dwUrlPathLength = _countof(UrlPath);
  uc.dwExtraInfoLength = _countof(ExtraInfo);
  std::wstring wurl = converter.from_bytes(url);
  if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), ICU_ESCAPE, &uc)) {
    return false;
  }
  DWORD flags = 0;
  switch (uc.nScheme) {
  case INTERNET_SCHEME_FTP:
    break;
  case INTERNET_SCHEME_HTTP:
    break;
  case INTERNET_SCHEME_HTTPS:
    flags |= WINHTTP_FLAG_SECURE;
    break;
  default:
    return false;
  }

  HINTERNET session = WinHttpOpen(L"Tangram UrlWorker", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (session == NULL) {
    return false;
  }

  HINTERNET hConnect = WinHttpConnect(session, uc.lpszHostName, uc.nPort, 0);
  if (hConnect == NULL) {
    WinHttpCloseHandle(session);
    return false;
  }
  std::wstring ObjectName = uc.lpszUrlPath;
  std::wstring strExtraInfo = uc.lpszExtraInfo;
  if (!strExtraInfo.empty()) {
    ObjectName = ObjectName + strExtraInfo;
  }

  HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", ObjectName.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (hRequest == NULL) {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(session);
    return false;
  }

  // send request
  bool noError = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != 0;
  noError = noError && (WinHttpReceiveResponse(hRequest, 0) != 0);
  wchar_t encoding[128] = { 0 };
  DWORD encoding_length = sizeof(encoding);
  if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_ENCODING, 0, encoding, &encoding_length, 0)) {
    // No Content Encoding is not a problem
    encoding_length = encoding_length / 2;
    encoding[encoding_length] = 0;
  }
  if (noError) {
    DWORD dwSize;
    char buffer[1024];
    do
    {
      dwSize = sizeof(buffer);

      if (WinHttpReadData(hRequest, (LPVOID)buffer,
        dwSize, &dwSize)) {
        size_t oldSize = stream.size();
        stream.resize(oldSize + dwSize);
        memcpy(stream.data() + oldSize, buffer, dwSize);
      }
      else {
        noError = false;
      }
      // Free the memory allocated to the buffer.
    } while (noError && dwSize > 0);
  }

  if (!noError) {
    printf("Error %u in WinHttpQueryDataAvailable.\n",
      GetLastError());
  }
  if (std::wstring(encoding) == L"gzip") {
    noError = ungzip(stream.data(), stream.size(), stream);
  }

  // close request
  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(session);
  return noError;
}